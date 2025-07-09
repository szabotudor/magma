#pragma once
#include "mgm_type_info.hpp"

#include <cassert>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>


namespace mgm {
    template<typename Callable = void>
    class MFunction {};

    template<typename Return, typename... Args>
    class MFunction<Return(Args...)> {
        struct Caller {
            virtual Return operator()(Args&&...) = 0;

            virtual Caller* copy_self() const = 0;

            virtual ~Caller() = default;
        };
        Caller* _caller = nullptr;

        template<typename CallableDirect>
        struct ActualCaller : public Caller {
            CallableDirect _direct;

            ActualCaller(const CallableDirect& direct)
                : _direct(direct) {}
            ActualCaller(CallableDirect& direct)
                : _direct(std::move(direct)) {}

            Return operator()(Args&&... args) override {
                return _direct(std::forward<Args>(args)...);
            }

            Caller* copy_self() const override {
                return new ActualCaller<CallableDirect>{_direct};
            }

            virtual ~ActualCaller() override = default;
        };

      public:
        MFunction() = default;

        MFunction(std::nullptr_t) {}

        MFunction(const MFunction& other) {
            if (other._caller)
                _caller = other._caller->copy_self();
        }
        MFunction(MFunction&& other)
            : _caller(other._caller) {
            other._caller = nullptr;
        }
        MFunction& operator=(const MFunction& other) {
            if (this == &other)
                return *this;

            delete _caller;
            if (other._caller)
                _caller = other._caller->copy_self();

            return *this;
        }
        MFunction& operator=(MFunction&& other) {
            if (this == &other)
                return *this;

            delete _caller;
            _caller = other._caller;
            other._caller = nullptr;

            return *this;
        }

        template<typename CallableDirect>
            requires std::is_invocable_r_v<Return, CallableDirect, Args...>
        MFunction(const CallableDirect& function) {
            _caller = new ActualCaller{function};
        }

        Return operator()(Args&&... args) const {
            return (*_caller)(std::forward<Args>(args)...);
        }

        operator bool() const {
            return _caller != nullptr;
        }

        ~MFunction() {
            delete _caller;
        }
    };


    template<>
    class MFunction<void> {
      public:
        struct ForcedReturn {
            void* data = nullptr;
            size_t type_id{};
            bool is_ptr = false;

            ForcedReturn(const ForcedReturn&) = delete;
            ForcedReturn& operator=(const ForcedReturn&) = delete;
            ForcedReturn& operator=(ForcedReturn&&) = delete;

            ForcedReturn(ForcedReturn&& other)
                : data(other.data),
                  type_id(other.type_id),
                  is_ptr(other.is_ptr) {
                other.data = nullptr;
                other.type_id = 0;
            }

            template<typename T>
            ForcedReturn(T&& raw)
                : data(new T{std::move(raw)}),
                  type_id(TypeID<T>{}),
                  is_ptr(std::is_pointer_v<T>) {}

            template<typename T>
                requires std::is_reference_v<T>
            ForcedReturn(T&& raw)
                : data(new std::remove_reference_t<T>* {&raw}),
                  type_id(TypeID<T>{}) {}

            ForcedReturn(void) = default;

            template<typename T>
            T get() {
                if (!is_ptr || (!std::is_same_v<T, void*>))
                    assert(TypeID<T>{} == type_id && "Casting to wrong type");

                T temp{std::move(*static_cast<T*>(data))};
                delete static_cast<T*>(data);
                data = nullptr;
                type_id = 0;

                return temp;
            }

            template<typename T>
                requires std::is_reference_v<T>
            T get() {
                if (!is_ptr || (!std::is_same_v<T, void*>))
                    assert(TypeID<T>{} == type_id && "Casting to wrong type");

                using Tnr = std::remove_reference_t<T>;

                Tnr* temp{*static_cast<Tnr**>(data)};
                delete static_cast<Tnr**>(data);
                data = nullptr;
                type_id = 0;

                return *temp;
            }

            ~ForcedReturn() {
                assert(data == nullptr && "You must use the return data before freeing the forced return");
            }
        };

      private:
        struct GenericCaller {
            virtual ForcedReturn forced_call(void** args) = 0;

            virtual GenericCaller* copy_self() const = 0;

            virtual ~GenericCaller() = default;
        };
        GenericCaller* _caller = nullptr;

        template<typename CallableDirect, typename Signature>
        struct ActualGenericCaller;

        template<typename CallableDirect, typename Return, typename... Args>
        struct ActualGenericCaller<CallableDirect, Return(Args...)> : public GenericCaller {
            CallableDirect _direct;

            ActualGenericCaller(const CallableDirect& direct)
                : _direct(direct) {}
            ActualGenericCaller(CallableDirect& direct)
                : _direct(std::move(direct)) {}

            template<std::size_t... Is>
            Return _forced_call(const std::index_sequence<Is...>&, void** args) {
                return _direct(*reinterpret_cast<std::remove_reference_t<Args>*>(args[Is])...);
            }

            ForcedReturn forced_call(void** args) override {
                if constexpr (std::is_same_v<Return, void>) {
                    _forced_call(std::index_sequence_for<Args...>{}, args);
                    return {};
                }
                else
                    return _forced_call(std::index_sequence_for<Args...>{}, args);
            }

            GenericCaller* copy_self() const override {
                return new ActualGenericCaller<CallableDirect, Return(Args...)>{_direct};
            }

            virtual ~ActualGenericCaller() override = default;
        };

        template<typename... Args, std::size_t... Is>
        static void _init(const std::index_sequence<Is...>&, std::tuple<Args..., ForcedReturn>* args_tuple, Args... args) {
            (new (&std::get<Is>(*args_tuple)) Args{std::forward<Args>(args)}, ...);
        }

      public:
        MFunction() = default;

        MFunction(const MFunction& other)
            : return_type_id(other.return_type_id),
              args_type_ids(other.args_type_ids) {
            if (other._caller)
                _caller = other._caller->copy_self();
        }
        MFunction(MFunction&& other)
            : _caller(other._caller),
              return_type_id(other.return_type_id),
              args_type_ids(std::move(other.args_type_ids)) {
            other._caller = nullptr;
            other.return_type_id = {};
            other.args_type_ids.clear();
        }
        MFunction& operator=(const MFunction& other) {
            if (this == &other)
                return *this;

            delete _caller;
            _caller = other._caller->copy_self();
            return_type_id = other.return_type_id;
            args_type_ids = other.args_type_ids;

            return *this;
        }
        MFunction& operator=(MFunction&& other) {
            if (this == &other)
                return *this;

            delete _caller;
            _caller = other._caller;
            other._caller = nullptr;
            return_type_id = other.return_type_id;
            other.return_type_id = {};
            args_type_ids = std::move(other.args_type_ids);
            other.args_type_ids.clear();

            return *this;
        }

        size_t return_type_id{};
        std::vector<size_t> args_type_ids{};

      private:
        template<typename Lambda>
        struct LambdaDetector;

        template<typename Class, typename Return, typename... Arguments>
        struct LambdaDetector<Return (Class::*)(Arguments...) const> {
            using Func = Return(Arguments...);
            using Ret = Return;
            using Args = std::tuple<Arguments...>;
        };
        template<typename Class, typename Return, typename... Arguments>
        struct LambdaDetector<Return (Class::*)(Arguments...)> {
            using Func = Return(Arguments...);
            using Ret = Return;
            using Args = std::tuple<Arguments...>;
        };

        template<typename T, typename = void>
        struct FullLambdaDetector {
            using Detect = void;
        };

        template<typename T>
        struct FullLambdaDetector<T, std::void_t<decltype(&T::operator())>> {
            using Detect = LambdaDetector<decltype(&T::operator())>;
        };

        template<typename T>
        using LambdaTraits = typename FullLambdaDetector<T>::Detect;

        template<typename T>
        static constexpr size_t type_id_list[0] = {};

        template<typename... Types>
        static constexpr size_t type_id_list<std::tuple<Types...>>[sizeof...(Types)] = {TypeID<Types>{}...};

      public:
        template<typename CallableDirect>
        MFunction(const CallableDirect& base_callable_object) {
            using Traits = LambdaTraits<CallableDirect>;

            if constexpr (std::is_void_v<Traits>) {
                static_assert(false, "base_callable_object must implement a non-templated call operator (must be a lambda, std::function, or similar)");
            }
            else {
                using Func = LambdaTraits<CallableDirect>::Func;
                using Ret = LambdaTraits<CallableDirect>::Ret;
                using Args = LambdaTraits<CallableDirect>::Args;

                _caller = new ActualGenericCaller<CallableDirect, Func>{base_callable_object};

                return_type_id = TypeID<Ret>{};
                args_type_ids = {type_id_list<Args>, type_id_list<Args> + std::tuple_size_v<Args>};
            }
        }

        template<typename Return, typename... Args>
        MFunction(Return (*const& base_function)(Args...)) {
            using Func = Return (*)(Args...);

            _caller = new ActualGenericCaller<Func, Return(Args...)>{base_function};

            return_type_id = TypeID<Return>{};
            args_type_ids = {TypeID<Args>{}...};
        }

        template<typename Class, typename Return, typename... Args>
        MFunction(Return (Class::* const& base_member_function_const)(Args...) const)
            : MFunction([base_member_function_const](const Class* _this, Args... args) {
                  if constexpr (std::is_void_v<Return>)
                      (_this->*base_member_function_const)(std::forward<Args>(args)...);
                  else
                      return (_this->*base_member_function_const)(std::forward<Args>(args)...);
              }) {
        }
        template<typename Class, typename Return, typename... Args>
        MFunction(Return (Class::* const& base_member_function)(Args...))
            : MFunction([base_member_function](Class* _this, Args... args) {
                  if constexpr (std::is_void_v<Return>)
                      (_this->*base_member_function)(std::forward<Args>(args)...);
                  else
                      return (_this->*base_member_function)(std::forward<Args>(args)...);
              }) {
        }

        /**
         * @brief Call the base callable using an array of pointers to arguments values
         *
         * @param args An array of pointers to the arguments that should be passed to the function
         * (WILL NOT RESPECT MOVE SEMANTICS FOR CONST ARGUMENTS. ARGUMENTS MIGHT BE FORWARDED/MOVED IF THE BASE CALLABLE HAS TO DO SO)
         * @return ForcedReturn A generic container for the return value
         */
        ForcedReturn forced_call(void** args) const {
            return _caller->forced_call(args);
        }

        template<typename... Args>
        ForcedReturn operator()(Args&&... args) const {
            void* args_ptr_list[sizeof...(Args)] = {(&args)...};
            return _caller->forced_call(args_ptr_list);
        }

        ~MFunction() {
            delete _caller;
        }
    };
} // namespace mgm
