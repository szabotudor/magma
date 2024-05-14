#pragma once
#include "editor.hpp"
#include "logging.hpp"
#include "systems.hpp"
#include "mgmath.hpp"

#include <cstddef>
#include <cstdint>
#include <unordered_map>


namespace mgm {
    class InspectorWindow : public EditorWindow {
        public:
        std::string name = "Inspector";

        InspectorWindow() = default;

        void draw_contents() override;

        ~InspectorWindow() = default;
    };

    class Inspector : public System {
        struct TypeInfo {
            std::string name{};
            std::function<bool(const std::string name, std::any& value)> inspector{};
        };
        std::unordered_map<size_t, TypeInfo> any_inspectors{};

        struct HoveredVectorInfo {
            std::string name{};
            float window_height = 0.0f;
        };

        std::vector<HoveredVectorInfo> hovered_vector_names{};

        public:
        template<typename T>
        void register_type_info(std::string name = {}, bool replace_existing = true) {
            if (name.empty())
                name = typeid(T).name();

            const auto it = any_inspectors.find(typeid(T).hash_code());
            if (it != any_inspectors.end()) {
                if (!replace_existing)
                    return;
                Logging{"Inspector"}.warning("Replacing already existing inspectable type \"", it->second.name, "\" with \"", name, "\"");
            }

            any_inspectors[typeid(T).hash_code()] = TypeInfo {
                .name = name,
                .inspector = [this](const std::string name, std::any& value) {
                    T& v = std::any_cast<T&>(value);
                    return inspect(name, v);
                }
            };
            any_inspectors[typeid(T&).hash_code()] = TypeInfo {
                .name = name,
                .inspector = [this](const std::string name, std::any& value) {
                    T& v = std::any_cast<T&>(value);
                    return inspect(name, v);
                }
            };
        }

        template<typename T>
        std::string get_type_name() {
            const auto it = any_inspectors.find(typeid(T).hash_code());
            if (it != any_inspectors.end())
                return it->second.name;
            register_type_info<T>();
            return any_inspectors.at(typeid(T).hash_code()).name;
        }

        Inspector();

        Inspector(const Inspector&) = delete;
        Inspector(Inspector&&) = delete;
        Inspector& operator=(const Inspector&) = delete;
        Inspector& operator=(Inspector&&) = delete;

        void on_begin_play() override;
        void update(float delta) override;
        void on_end_play() override;

        template<typename T, typename S = T>
        struct MinMaxNum {
            T& value;
            S min, max, speed;
            MinMaxNum(T& value, S min, S max, S speed = static_cast<S>(1))
                : value(value), min(min), max(max), speed(speed) {}
        };

        template<typename T>
        bool inspect(const std::string& name, T& value) {
            (void)name; (void)value;
            Logging{"Inspector"}.warning("No inspector for type: ", get_type_name<T>());
            return false;
        }
        template<typename T>
        void inspect(const std::string& name, const T& value) const {
            auto* inspector = const_cast<Inspector*>(this);
            auto dup = value;
            inspector->inspect(name, dup);
        }

        template<typename T, typename S = T>
        bool inspect(const std::string& name, T& value, const S& min, const S& max, const S& speed = static_cast<S>(1)) {
            return inspect<T>(name, MinMaxNum<T, S>{value, min, max, speed});
        }

        private:
        size_t vector_depth = 0, max_vector_depth = 0;
        bool begin_vector(std::string name, bool has_elements);
        void end_vector();

        public:

        template<typename T>
        bool inspect(const std::string& name, std::vector<T>& values) {
            bool edited = false;
            if (begin_vector(name, !values.empty())) {
                for (auto& v : values)
                    edited |= inspect(get_type_name<T>() + std::to_string(&v - &*values.begin()), v);
                end_vector();
            }
            return edited;
        }
        
        template<typename T>
        bool inspect(const std::string& name, std::vector<T>& values, const T& min, const T& max, const T& speed = static_cast<T>(1)) {
            bool edited = false;
            if (begin_vector(name, !values.empty())) {
                for (auto& v : values)
                    edited |= inspect(get_type_name<T>() + std::to_string(&v - &*values.begin()), v, min, max, speed);
                end_vector();
            }
            return edited;
        }

        template<template <typename, typename> typename MapType, typename T>
        bool inspect(const std::string& name, MapType<std::string, T>& values) {
            bool edited = false;
            if (begin_vector(name, !values.empty())) {
                for (auto& [a, b] : values)
                    edited |= inspect(a, b);
                end_vector();
            }
            return edited;
        }

        template<template <typename, typename> typename MapType, typename T>
        bool inspect(const std::string& name, MapType<std::string, T>& values, const T& min, const T& max, const T& speed = static_cast<T>(1)) {
            bool edited = false;
            if (begin_vector(name, !values.empty())) {
                for (auto& [a, b] : values)
                    edited |= inspect(a, b, min, max, speed);
                end_vector();
            }
            return edited;
        }

        ~Inspector() override = default;
    };

    template<> bool Inspector::inspect<int>(const std::string& name, int& value);
    template<> bool Inspector::inspect<Inspector::MinMaxNum<int>>(const std::string& name, MinMaxNum<int>& value);
    template<> bool Inspector::inspect<uint32_t>(const std::string& name, uint32_t& value);
    template<> bool Inspector::inspect<Inspector::MinMaxNum<uint32_t>>(const std::string& name, MinMaxNum<uint32_t>& value);
    template<> bool Inspector::inspect<float>(const std::string& name, float& value);
    template<> bool Inspector::inspect<Inspector::MinMaxNum<float>>(const std::string& name, MinMaxNum<float>& value);
    template<> bool Inspector::inspect<double>(const std::string& name, double& value);
    template<> bool Inspector::inspect<Inspector::MinMaxNum<double>>(const std::string& name, MinMaxNum<double>& value);
    template<> bool Inspector::inspect<vec2f>(const std::string& name, vec2f& value);
    template<> bool Inspector::inspect<vec3f>(const std::string& name, vec3f& value);
    template<> bool Inspector::inspect<vec4f>(const std::string& name, vec4f& value);
    template<> bool Inspector::inspect<Inspector::MinMaxNum<vec2f, float>>(const std::string& name, MinMaxNum<vec2f, float>& value);
    template<> bool Inspector::inspect<Inspector::MinMaxNum<vec3f, float>>(const std::string& name, MinMaxNum<vec3f, float>& value);
    template<> bool Inspector::inspect<Inspector::MinMaxNum<vec4f, float>>(const std::string& name, MinMaxNum<vec4f, float>& value);
    template<> bool Inspector::inspect<vec2d>(const std::string& name, vec2d& value);
    template<> bool Inspector::inspect<vec3d>(const std::string& name, vec3d& value);
    template<> bool Inspector::inspect<vec4d>(const std::string& name, vec4d& value);
    template<> bool Inspector::inspect<Inspector::MinMaxNum<vec2d, double>>(const std::string& name, MinMaxNum<vec2d, double>& value);
    template<> bool Inspector::inspect<Inspector::MinMaxNum<vec3d, double>>(const std::string& name, MinMaxNum<vec3d, double>& value);
    template<> bool Inspector::inspect<Inspector::MinMaxNum<vec4d, double>>(const std::string& name, MinMaxNum<vec4d, double>& value);
    template<> bool Inspector::inspect<vec2i32>(const std::string& name, vec2i32& value);
    template<> bool Inspector::inspect<vec3i32>(const std::string& name, vec3i32& value);
    template<> bool Inspector::inspect<vec4i32>(const std::string& name, vec4i32& value);
    template<> bool Inspector::inspect<Inspector::MinMaxNum<vec2i32, float>>(const std::string& name, MinMaxNum<vec2i32, float>& value);
    template<> bool Inspector::inspect<Inspector::MinMaxNum<vec3i32, float>>(const std::string& name, MinMaxNum<vec3i32, float>& value);
    template<> bool Inspector::inspect<Inspector::MinMaxNum<vec4i32, float>>(const std::string& name, MinMaxNum<vec4i32, float>& value);
    template<> bool Inspector::inspect<vec2u32>(const std::string& name, vec2u32& value);
    template<> bool Inspector::inspect<vec3u32>(const std::string& name, vec3u32& value);
    template<> bool Inspector::inspect<vec4u32>(const std::string& name, vec4u32& value);
    template<> bool Inspector::inspect<Inspector::MinMaxNum<vec2u32, float>>(const std::string& name, MinMaxNum<vec2u32, float>& value);
    template<> bool Inspector::inspect<Inspector::MinMaxNum<vec3u32, float>>(const std::string& name, MinMaxNum<vec3u32, float>& value);
    template<> bool Inspector::inspect<Inspector::MinMaxNum<vec4u32, float>>(const std::string& name, MinMaxNum<vec4u32, float>& value);
    template<> bool Inspector::inspect<std::string>(const std::string& name, std::string& value);
    template<> bool Inspector::inspect<bool>(const std::string& name, bool& value);
    template<> bool Inspector::inspect<mat2f>(const std::string& name, mat2f& value);
    template<> bool Inspector::inspect<mat3f>(const std::string& name, mat3f& value);
    template<> bool Inspector::inspect<mat4f>(const std::string& name, mat4f& value);
    template<> bool Inspector::inspect<Inspector::MinMaxNum<mat2f, float>>(const std::string& name, MinMaxNum<mat2f, float>& value);
    template<> bool Inspector::inspect<Inspector::MinMaxNum<mat3f, float>>(const std::string& name, MinMaxNum<mat3f, float>& value);
    template<> bool Inspector::inspect<Inspector::MinMaxNum<mat4f, float>>(const std::string& name, MinMaxNum<mat4f, float>& value);
    template<> bool Inspector::inspect<mat2d>(const std::string& name, mat2d& value);
    template<> bool Inspector::inspect<mat3d>(const std::string& name, mat3d& value);
    template<> bool Inspector::inspect<mat4d>(const std::string& name, mat4d& value);
    template<> bool Inspector::inspect<Inspector::MinMaxNum<mat2d, double>>(const std::string& name, MinMaxNum<mat2d, double>& value);
    template<> bool Inspector::inspect<Inspector::MinMaxNum<mat3d, double>>(const std::string& name, MinMaxNum<mat3d, double>& value);
    template<> bool Inspector::inspect<Inspector::MinMaxNum<mat4d, double>>(const std::string& name, MinMaxNum<mat4d, double>& value);
}
