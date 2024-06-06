#pragma once
#include "editor.hpp"
#include "helpers.hpp"


namespace mgm {
    template <typename T>
    struct MemberFunctionTraits;
    template <typename ReturnType, typename ClassType, typename... Args>
    struct MemberFunctionTraits<ReturnType(ClassType::*)(Args...)> {
        using Class = ClassType;
    };

    struct RegisterSubsectionFunction {
        friend class EditorSettings;

        using SectionsMap = std::unordered_map<size_t, std::unordered_map<std::string, std::function<void(System*)>>>;

        static SectionsMap& get_subsections_map() {
            static SectionsMap subsections{};
            return subsections;
        }

        template<typename T>
        RegisterSubsectionFunction(const std::string& name, const T& func) {
            using Class = typename MemberFunctionTraits<T>::Class;
            const auto type_id = typeid(Class).hash_code();
            get_subsections_map()[type_id][name] = [func](System* sys) {
                (((Class*)sys)->*func)();
            };
        }
    };

    #define SETTINGS_SUBSECTION_DRAW_FUNC(function) \
    DEFINE_SELF_WITH_NAME(Self_##function) \
    static inline RegisterSubsectionFunction __register_subsection_function__##function{#function, &Self_##function::function}; \

    class EditorSettings : public EditorWindow {
        size_t selected_system = 0;

    public:
        EditorSettings() {
            window_name = "Settings";
        }

        void draw_contents() override;
    };
}
