#pragma once
#include <cassert>
#include <cstdint>
#include <queue>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

#include "any.hpp"
#include "helpers.hpp"


namespace mgm {
    // On MSVC, the [[no_unique_address]] attribute is replaced with [[msvc::no_unique_address]]
    #if defined(_MSC_VER)
    #define __impl_no_unique_address [[msvc::no_unique_address]]
    #else
    #define __impl_no_unique_address [[no_unique_address]]
    #endif

    class ExposeApiRuntime;

    struct ExposeApi {
        enum class ExposeTimeMode {
            CLASS, FUNCTION, VARIABLE
        };


        struct ExposedClasses {
            struct ExposedFunction {
                std::function<Any(void*, std::vector<Any>)> function{};
                
                struct Signature {
                    std::string traits_and_qualifiers{};
                    std::string return_type{};
                    std::string name{};
                    size_t return_type_id{};
                    
                    struct Argument {
                        std::string traits_and_qualifiers{};
                        std::string type{};
                        std::string name{};
                        size_t type_id{};
                    };
                    std::vector<Argument> arguments{};
                } signature{};

                ExposedFunction(std::function<Any(void*, std::vector<Any>)> init_function = {})
                    : function{init_function} {}

                Any operator()(void* object, std::vector<Any> args) {
                    return function(object, args);
                }
                Any operator()(void* object, std::vector<Any> args) const {
                    return function(object, args);
                }
            };
            struct ExposedVariable {
                uintptr_t offset{};
                std::string name{};

                ExposedVariable(uintptr_t member_offset, const std::string& member_name)
                    : offset{member_offset}, name{member_name} {}

                template<typename T>
                T& get(void* object) {
                    return *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(object) + offset);
                }
                template<typename T>
                const T& get_const(const void* object) {
                    return *reinterpret_cast<const T*>(reinterpret_cast<uintptr_t>(object) + offset);
                }
            };
            struct ExposedMember {
                std::unordered_map<size_t, ExposedFunction>* functions{};
                ExposedVariable* variable = nullptr;
                bool is_function = false, has_value = false;
                
                template<typename... Ts>
                ExposedFunction& emplace_function(size_t type_id, Ts&&... args) {
                    if (has_value && !is_function)
                        throw std::runtime_error("Member already has a variable value");
                    if (!has_value)
                        functions = new std::unordered_map<size_t, ExposedFunction>{};
                    is_function = true;
                    has_value = true;
                    (*functions)[type_id] = ExposedFunction{std::forward<Ts>(args)...};
                    return functions->at(type_id);
                }
                template<typename... Ts>
                void emplace_variable(Ts&&... args) {
                    if (has_value)
                        return;
                    is_function = false;
                    has_value = true;
                    variable = new ExposedVariable{std::forward<Ts>(args)...};
                }

                void reset() {
                    if (!is_function)
                        delete variable;
                    is_function = false;
                    has_value = false;
                }

                ~ExposedMember() {
                    reset();
                }
            };
            struct ExposedClassMembers {
                std::unordered_map<std::string, ExposedMember> members{};
                uintptr_t runtime_offset = 0;

                struct NextFunction {
                    std::string name{};
                    size_t type_id{};

                    bool operator<(const NextFunction& other) const {
                        return name < other.name;
                    }
                    bool operator>(const NextFunction& other) const {
                        return name > other.name;
                    }
                };
                std::priority_queue<NextFunction, std::vector<NextFunction>> args_queue{};
            };
            std::unordered_map<size_t, std::string> class_names{};
            std::unordered_map<size_t, ExposedClassMembers> class_members{};
        };
        static ExposedClasses& get_exposed_classes() {
            static ExposedClasses expose_data{};
            return expose_data;
        }


        ExposeApi() = default;

        template<typename T>
        struct ExposedClassAnalyzer {
            ExposedClassAnalyzer();

            template<typename ReturnType, typename... Args>
            static bool expose(ReturnType(T::* function)(Args...), const std::string& name) {
                using TypeErasedFunctor = ReturnType(Args...);
                auto& expose_data = ExposeApi::get_exposed_classes();                
                const size_t class_id = typeid(T).hash_code();
                auto& new_function = expose_data.class_members[class_id].members[name].emplace_function(typeid(TypeErasedFunctor).hash_code(), ExposedClasses::ExposedFunction{
                    [function](void* object, std::vector<Any> args) -> Any{
                        assert(args.size() == sizeof...(Args) && "Invalid number of arguments for function call");
                        if constexpr (sizeof...(Args) != 0) {
                            size_t i = sizeof...(Args) - 1;
                            auto get_arg = [&args, &i]<typename Arg>() -> Arg {
                                return args[i--].get<Arg>();
                            };
                            if constexpr (!std::is_void_v<ReturnType>)
                                return Any((reinterpret_cast<T*>(object)->*function)(std::forward<Args>(get_arg.template operator()<Args>())...));
                            (reinterpret_cast<T*>(object)->*function)(std::forward<Args>(get_arg.template operator()<Args>())...);
                            return {};
                        }
                        else {
                            if constexpr (!std::is_void_v<ReturnType>)
                                return Any((reinterpret_cast<T*>(object)->*function)());
                            (reinterpret_cast<T*>(object)->*function)();
                            return {};
                        }
                    }
                });
                new_function.signature.name = name;
                new_function.signature.return_type_id = typeid(ReturnType).hash_code();
                expose_data.class_members[class_id].args_queue.push(ExposedClasses::ExposedClassMembers::NextFunction{
                    .name = name,
                    .type_id = typeid(TypeErasedFunctor).hash_code()
                });
                return true;
            }

            template<typename... Args>
            static bool expose_args(void (*)(Args...), const std::vector<std::string>& types_and_names) {
                auto& expose_data = ExposeApi::get_exposed_classes();
                const auto class_id = typeid(T).hash_code();
                const auto& member_to_expose = expose_data.class_members[class_id].args_queue.top();
                auto& member = expose_data.class_members[class_id].members[member_to_expose.name];
                const auto member_id = member_to_expose.type_id;
                expose_data.class_members[class_id].args_queue.pop();

                std::vector<size_t> type_ids{};
                type_ids.reserve(sizeof...(Args));
                (type_ids.push_back(typeid(Args).hash_code()), ...);

                for (size_t i = 0; i < types_and_names.size(); i++) {
                    const auto& arg = types_and_names[i];
                    const auto space_pos = arg.find(' ');
                    const auto type = arg.substr(0, space_pos);
                    const auto name = arg.substr(space_pos + 1);
                    auto& sig = (*member.functions)[member_id].signature;
                    sig.arguments.push_back(ExposedClasses::ExposedFunction::Signature::Argument{
                        .type = type,
                        .name = name,
                        .type_id = type_ids.back()
                    });
                }

                return true;
            }

            template<typename Var, std::enable_if_t<!std::is_function_v<Var>, bool> = true>
            static bool expose([[maybe_unused]] Var T::* variable, const std::string& name) {
                auto& expose_data = ExposeApi::get_exposed_classes();
                const size_t class_id = typeid(T).hash_code();

                const auto offset = *reinterpret_cast<uintptr_t*>(&variable);
                expose_data.class_members[class_id].members[name].emplace_variable(offset, name);

                return true;
            }
        };

        template<typename T>
        static ExposeApi expose_class(const std::string& name = "NoName") {
            auto& expose_data = ExposeApi::get_exposed_classes();
            expose_data.class_names[typeid(T).hash_code()] = name;
            return {};
        }
    };

    class ExposeApiRuntime {
    public:
        static size_t get_class_id(ExposeApiRuntime* instance) {
            return typeid(*instance).hash_code();
        }

        struct AutoExposedMemberFinder {
            void* object = nullptr;
            ExposeApi::ExposedClasses::ExposedMember member{};

            template<typename ReturnType, typename... Args>
            ReturnType call(Args&&... args) {
                if (!member.has_value || !member.is_function)
                    throw std::runtime_error("Member is not a function");
                auto func = member.functions->at(typeid(ReturnType(Args...)).hash_code());
                auto res = func(object, {std::forward<Args>(args)...});
                if constexpr (!std::is_same_v<ReturnType, void>)
                    return res.template get<ReturnType>();
            }

            template<typename ReturnType, typename... Args>
            ReturnType call(Args&&... args) const {
                if (!member.has_value || !member.is_function)
                    throw std::runtime_error("Member is not a function");
                const auto func = member.functions->at(typeid(ReturnType(Args...)).hash_code());
                const auto res = func(object, {std::forward<Args>(args)...});
                if constexpr (!std::is_same_v<ReturnType, void>)
                    return res.template get<ReturnType>();
            }

            template<typename M>
            M& get() {
                if (!member.has_value || member.is_function)
                    throw std::runtime_error("Member is not a variable");
                return member.variable->get<M>(object);
            }

            template<typename M>
            const M& get() const {
                if (!member.has_value || member.is_function)
                    throw std::runtime_error("Member is not a variable");
                return member.variable->get_const<M>(object);
            }
            
            ExposeApi::ExposedClasses::ExposedFunction::Signature get_signature() const {
                if (!member.has_value || !member.is_function)
                    throw std::runtime_error("Member is not a function");
                return member.functions->begin()->second.signature;
            }

            bool is_function() const { return member.is_function; }
            bool is_variable() const { return !member.is_function; }
        };
        virtual AutoExposedMemberFinder get_member(const std::string& name) {
            auto& expose_data = ExposeApi::get_exposed_classes();
            const auto type_id = get_class_id(this);
            const auto offset = expose_data.class_members[type_id].runtime_offset;
            const auto _this = reinterpret_cast<uintptr_t>(this) - offset;
            return {
                .object = reinterpret_cast<void*>(_this),
                .member = expose_data.class_members[type_id].members[name]
            };
        }
        virtual std::unordered_multimap<std::string, AutoExposedMemberFinder> get_all_members() {
            std::unordered_multimap<std::string, AutoExposedMemberFinder> members{};
            auto& expose_data = ExposeApi::get_exposed_classes();
            const auto type_id = get_class_id(this);

            for (const auto& [name, member] : expose_data.class_members[type_id].members) {
                if (name == "__expose_api_member_offset_initializer")
                    continue;
                members.emplace(name, AutoExposedMemberFinder{
                    .object = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(this) - expose_data.class_members[type_id].runtime_offset),
                    .member = member
                });
            }

            return members;
        }

        virtual ~ExposeApiRuntime() = default;
    };

    template<typename T>
    ExposeApi::ExposedClassAnalyzer<T>::ExposedClassAnalyzer() {
        const auto __offset = &T::__expose_api_analyzer;
        const auto offset = *reinterpret_cast<const uintptr_t*>(&__offset);
        assert(offset == 0 && "Class must not have any members before using the EXPOSE_CLASS() macro");

        const auto __T_instance = (void*)this;
        T* T_instance = *reinterpret_cast<T* const*>(&__T_instance);
        ExposeApiRuntime::AutoExposedMemberFinder empty_member{};
        if constexpr (std::is_base_of_v<ExposeApiRuntime, T>)
            empty_member = T_instance->get_member("__expose_api_member_offset_initializer");
        else
            empty_member = T_instance->static_get_member("__expose_api_member_offset_initializer");
        const auto real_offset = reinterpret_cast<uintptr_t>(empty_member.object) - reinterpret_cast<uintptr_t>(T_instance);
        ExposeApi::get_exposed_classes().class_members[typeid(T).hash_code()].runtime_offset = real_offset;
    }

    #define __STR2(x) #x
    #define __STR(x) __STR2(x)
    #define __CAT2(a, b) a##b
    #define __CAT(a, b) __CAT2(a, b)


    #define MARGS(...) \
        (__VA_ARGS__); \
        static inline void __CAT(__expose_api_temp_function, __LINE__) (__VA_ARGS__) {} \
        static inline const bool __CAT(__expose_api_args_helper_, __LINE__) = ExposeApi::ExposedClassAnalyzer<__ExposeApi_Self>::expose_args(&__ExposeApi_Self::__CAT(__expose_api_temp_function, __LINE__), {FOR_EACH(__STR, __VA_ARGS__)})
    
    #define MFUNC_NO_AUTO_ARGS(member, ...) \
        static inline* const __expose_api_return_type_##member{}; \
        static inline bool __expose_api_initializer_function_##member() { \
            return ExposeApi::ExposedClassAnalyzer<__ExposeApi_Self>::expose(&__ExposeApi_Self::member, std::string(#__VA_ARGS__).empty() ? #member : #__VA_ARGS__); \
        } \
        static inline const bool __expose_api_temp_helper_##member = __expose_api_initializer_function_##member(); \
        std::remove_reference_t<std::remove_pointer_t<decltype(__expose_api_return_type_##member)>> member

    #define MFUNC(member, ...) \
        MFUNC_NO_AUTO_ARGS(member, __VA_ARGS__) MARGS
    

    #define MVAR(member, ...) \
        static inline* const __expose_api_var_type_##member{}; \
        static inline bool __expose_api_initializer_var_##member() { \
            return ExposeApi::ExposedClassAnalyzer<__ExposeApi_Self>::expose(&__ExposeApi_Self::member, std::string(#__VA_ARGS__).empty() ? #member : #__VA_ARGS__); \
        } \
        static inline const bool __expose_api_temp_helper_##member = __expose_api_initializer_var_##member(); \
        std::remove_reference_t<std::remove_pointer_t<decltype(__expose_api_var_type_##member)>> member


    #define EXPOSE_CLASS(class_name) \
        DEFINE_SELF_WITH_NAME(__ExposeApi_Self) \
        __impl_no_unique_address ExposeApi::ExposedClassAnalyzer<__ExposeApi_Self> __expose_api_analyzer; \
        \
        template<typename __ExposeApi_T = __ExposeApi_Self, std::enable_if_t<!std::is_base_of_v<ExposeApiRuntime, __ExposeApi_T>, bool> = true> \
        ExposeApiRuntime::AutoExposedMemberFinder static_get_member(const std::string& name) { \
            return { \
                .object = this, \
                .member = ExposeApi::get_exposed_classes().class_members[typeid(__ExposeApi_Self).hash_code()].members[name] \
            }; \
        } \
        \
        template<typename __ExposeApi_T = __ExposeApi_Self, std::enable_if_t<!std::is_base_of_v<ExposeApiRuntime, __ExposeApi_T>, bool> = true> \
        std::unordered_multimap<std::string, ExposeApiRuntime::AutoExposedMemberFinder> static_get_all_members() { \
            std::unordered_multimap<std::string, ExposeApiRuntime::AutoExposedMemberFinder> members{}; \
            auto& expose_data = ExposeApi::get_exposed_classes(); \
            \
            for (const auto& [name, member] : expose_data.class_members[typeid(__ExposeApi_Self).hash_code()].members) { \
                members.emplace(name, ExposeApiRuntime::AutoExposedMemberFinder{ \
                    .object = this, \
                    .member = member \
                }); \
            } \
            \
            return members; \
        } \
        static inline ExposeApi __expose_api_class_auto_initializer{ExposeApi::expose_class<__ExposeApi_Self>(class_name)}; \
        \
        void MFUNC_NO_AUTO_ARGS(__expose_api_member_offset_initializer)() {}
}
