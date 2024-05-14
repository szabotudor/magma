#include "inspector.hpp"
#include "engine.hpp"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "mgmath.hpp"
#include <cstdint>
#include <string>
#include <utility>


namespace mgm {
    void InspectorWindow::draw_contents() {
    }


    template<> bool Inspector::inspect<int>(const std::string& name, int& value) {
        return ImGui::InputInt(name.c_str(), &value);
    }
    template<> bool Inspector::inspect<Inspector::MinMaxNum<int>>(const std::string& name, MinMaxNum<int>& value) {
        return ImGui::DragInt(name.c_str(), &value.value, static_cast<float>(value.speed), value.min, value.max);
    }

    template<> bool Inspector::inspect<uint32_t>(const std::string& name, uint32_t& value) {
        return ImGui::InputScalar(name.c_str(), ImGuiDataType_U32, &value);
    }
    template<> bool Inspector::inspect<Inspector::MinMaxNum<uint32_t>>(const std::string& name, MinMaxNum<uint32_t>& value) {
        return ImGui::DragScalar(name.c_str(), ImGuiDataType_U32, &value.value, static_cast<float>(value.speed), &value.min, &value.max);
    }

    template<> bool Inspector::inspect<float>(const std::string& name, float& value) {
        return ImGui::InputFloat(name.c_str(), &value);
    }
    template<> bool Inspector::inspect<Inspector::MinMaxNum<float>>(const std::string& name, MinMaxNum<float>& value) {
        return ImGui::DragFloat(name.c_str(), &value.value, value.speed, value.min, value.max);
    }

    template<> bool Inspector::inspect<double>(const std::string& name, double& value) {
        return ImGui::InputDouble(name.c_str(), &value);
    }
    template<> bool Inspector::inspect<Inspector::MinMaxNum<double>>(const std::string& name, MinMaxNum<double>& value) {
        return ImGui::DragScalar(name.c_str(), ImGuiDataType_Double, &value.value, static_cast<float>(value.speed), &value.min, &value.max);
    }

    template<> bool Inspector::inspect<vec2f>(const std::string& name, vec2f& value) {
        return ImGui::InputFloat2(name.c_str(), &value.x());
    }
    template<> bool Inspector::inspect<vec3f>(const std::string& name, vec3f& value) {
        return ImGui::InputFloat3(name.c_str(), &value.x());
    }
    template<> bool Inspector::inspect<vec4f>(const std::string& name, vec4f& value) {
        return ImGui::InputFloat4(name.c_str(), &value.x());
    }

    template<> bool Inspector::inspect<Inspector::MinMaxNum<vec2f, float>>(const std::string& name, MinMaxNum<vec2f, float>& value) {
        return ImGui::DragFloat2(name.c_str(), &value.value.x(), value.speed, value.min, value.max);
    }
    template<> bool Inspector::inspect<Inspector::MinMaxNum<vec3f, float>>(const std::string& name, MinMaxNum<vec3f, float>& value) {
        return ImGui::DragFloat3(name.c_str(), &value.value.x(), value.speed, value.min, value.max);
    }
    template<> bool Inspector::inspect<Inspector::MinMaxNum<vec4f, float>>(const std::string& name, MinMaxNum<vec4f, float>& value) {
        return ImGui::DragFloat4(name.c_str(), &value.value.x(), value.speed, value.min, value.max);
    }

    template<> bool Inspector::inspect<vec2d>(const std::string& name, vec2d& value) {
        return ImGui::InputScalarN(name.c_str(), ImGuiDataType_Double, &value.x(), 2);
    }
    template<> bool Inspector::inspect<vec3d>(const std::string& name, vec3d& value) {
        return ImGui::InputScalarN(name.c_str(), ImGuiDataType_Double, &value.x(), 3);
    }
    template<> bool Inspector::inspect<vec4d>(const std::string& name, vec4d& value) {
        return ImGui::InputScalarN(name.c_str(), ImGuiDataType_Double, &value.x(), 4);
    }

    template<> bool Inspector::inspect<Inspector::MinMaxNum<vec2d, double>>(const std::string& name, MinMaxNum<vec2d, double>& value) {
        return ImGui::DragScalarN(name.c_str(), ImGuiDataType_Double, &value.value.x(), 2, static_cast<float>(value.speed), &value.min, &value.max);
    }
    template<> bool Inspector::inspect<Inspector::MinMaxNum<vec3d, double>>(const std::string& name, MinMaxNum<vec3d, double>& value) {
        return ImGui::DragScalarN(name.c_str(), ImGuiDataType_Double, &value.value.x(), 3, static_cast<float>(value.speed), &value.min, &value.max);
    }
    template<> bool Inspector::inspect<Inspector::MinMaxNum<vec4d, double>>(const std::string& name, MinMaxNum<vec4d, double>& value) {
        return ImGui::DragScalarN(name.c_str(), ImGuiDataType_Double, &value.value.x(), 4, static_cast<float>(value.speed), &value.min, &value.max);
    }

    template<> bool Inspector::inspect<vec2i32>(const std::string& name, vec2i32& value) {
        return ImGui::InputInt2(name.c_str(), &value.x());
    }
    template<> bool Inspector::inspect<vec3i32>(const std::string& name, vec3i32& value) {
        return ImGui::InputInt3(name.c_str(), &value.x());
    }
    template<> bool Inspector::inspect<vec4i32>(const std::string& name, vec4i32& value) {
        return ImGui::InputInt4(name.c_str(), &value.x());
    }

    template<> bool Inspector::inspect<Inspector::MinMaxNum<vec2i32, int>>(const std::string& name, MinMaxNum<vec2i32, int>& value) {
        return ImGui::DragInt2(name.c_str(), &value.value.x(), static_cast<float>(value.speed), value.min, value.max);
    }
    template<> bool Inspector::inspect<Inspector::MinMaxNum<vec3i32, int>>(const std::string& name, MinMaxNum<vec3i32, int>& value) {
        return ImGui::DragInt3(name.c_str(), &value.value.x(), static_cast<float>(value.speed), value.min, value.max);
    }
    template<> bool Inspector::inspect<Inspector::MinMaxNum<vec4i32, int>>(const std::string& name, MinMaxNum<vec4i32, int>& value) {
        return ImGui::DragInt4(name.c_str(), &value.value.x(), static_cast<float>(value.speed), value.min, value.max);
    }

    template<> bool Inspector::inspect<vec2u32>(const std::string& name, vec2u32& value) {
        return ImGui::InputScalarN(name.c_str(), ImGuiDataType_U32, &value.x(), 2);
    }
    template<> bool Inspector::inspect<vec3u32>(const std::string& name, vec3u32& value) {
        return ImGui::InputScalarN(name.c_str(), ImGuiDataType_U32, &value.x(), 3);
    }
    template<> bool Inspector::inspect<vec4u32>(const std::string& name, vec4u32& value) {
        return ImGui::InputScalarN(name.c_str(), ImGuiDataType_U32, &value.x(), 4);
    }

    template<> bool Inspector::inspect<Inspector::MinMaxNum<vec2u32, uint32_t>>(const std::string& name, MinMaxNum<vec2u32, uint32_t>& value) {
        return ImGui::DragScalarN(name.c_str(), ImGuiDataType_U32, &value.value.x(), 2, static_cast<float>(value.speed), &value.min, &value.max);
    }
    template<> bool Inspector::inspect<Inspector::MinMaxNum<vec3u32, uint32_t>>(const std::string& name, MinMaxNum<vec3u32, uint32_t>& value) {
        return ImGui::DragScalarN(name.c_str(), ImGuiDataType_U32, &value.value.x(), 3, static_cast<float>(value.speed), &value.min, &value.max);
    }
    template<> bool Inspector::inspect<Inspector::MinMaxNum<vec4u32, uint32_t>>(const std::string& name, MinMaxNum<vec4u32, uint32_t>& value) {
        return ImGui::DragScalarN(name.c_str(), ImGuiDataType_U32, &value.value.x(), 4, static_cast<float>(value.speed), &value.min, &value.max);
    }

    template<> bool Inspector::inspect<std::string>(const std::string& name, std::string& value) {
        return ImGui::InputText(name.c_str(), &value);
    }

    template<> bool Inspector::inspect<bool>(const std::string& name, bool& value) {
        return ImGui::Checkbox(name.c_str(), &value);
    }

    template<> bool Inspector::inspect<mat2f>(const std::string& name, mat2f& value) {
        ImGui::Text("%s", name.c_str());
        ImGui::Indent();
        bool edited = ImGui::InputFloat2("row #1", &value[0].x());
        edited |= ImGui::InputFloat2("row #2", &value[1].x());
        ImGui::Unindent();
        return edited;
    }
    template<> bool Inspector::inspect<mat3f>(const std::string& name, mat3f& value) {
        ImGui::Text("%s", name.c_str());
        ImGui::Indent();
        bool edited = ImGui::InputFloat3("row #1", &value[0].x());
        edited |= ImGui::InputFloat3("row #2", &value[1].x());
        edited |= ImGui::InputFloat3("row #3", &value[2].x());
        ImGui::Unindent();
        return edited;
    }
    template<> bool Inspector::inspect<mat4f>(const std::string& name, mat4f& value) {
        ImGui::Text("%s", name.c_str());
        ImGui::Indent();
        bool edited = ImGui::InputFloat4("row #1", &value[0].x());
        edited |= ImGui::InputFloat4("row #2", &value[1].x());
        edited |= ImGui::InputFloat4("row #3", &value[2].x());
        edited |= ImGui::InputFloat4("row #4", &value[3].x());
        ImGui::Unindent();
        return edited;
    }

    template<> bool Inspector::inspect<Inspector::MinMaxNum<mat2f, float>>(const std::string& name, MinMaxNum<mat2f, float>& value) {
        ImGui::Text("%s", name.c_str());
        ImGui::Indent();
        bool edited = ImGui::DragFloat2("row #1", &value.value[0].x(), value.speed, value.min, value.max);
        edited |= ImGui::DragFloat2("row #2", &value.value[1].x(), value.speed, value.min, value.max);
        ImGui::Unindent();
        return edited;
    }
    template<> bool Inspector::inspect<Inspector::MinMaxNum<mat3f, float>>(const std::string& name, MinMaxNum<mat3f, float>& value) {
        ImGui::Text("%s", name.c_str());
        ImGui::Indent();
        bool edited = ImGui::DragFloat3("row #1", &value.value[0].x(), value.speed, value.min, value.max);
        edited |= ImGui::DragFloat3("row #2", &value.value[1].x(), value.speed, value.min, value.max);
        edited |= ImGui::DragFloat3("row #3", &value.value[2].x(), value.speed, value.min, value.max);
        ImGui::Unindent();
        return edited;
    }
    template<> bool Inspector::inspect<Inspector::MinMaxNum<mat4f, float>>(const std::string& name, MinMaxNum<mat4f, float>& value) {
        ImGui::Text("%s", name.c_str());
        ImGui::Indent();
        bool edited = ImGui::DragFloat4("row #1", &value.value[0].x(), value.speed, value.min, value.max);
        edited |= ImGui::DragFloat4("row #2", &value.value[1].x(), value.speed, value.min, value.max);
        edited |= ImGui::DragFloat4("row #3", &value.value[2].x(), value.speed, value.min, value.max);
        edited |= ImGui::DragFloat4("row #4", &value.value[3].x(), value.speed, value.min, value.max);
        ImGui::Unindent();
        return edited;
    }

    template<> bool Inspector::inspect<mat2d>(const std::string& name, mat2d& value) {
        ImGui::Text("%s", name.c_str());
        ImGui::Indent();
        bool edited = ImGui::InputScalarN("row #1", ImGuiDataType_Double, &value[0].x(), 2);
        edited |= ImGui::InputScalarN("row #2", ImGuiDataType_Double, &value[1].x(), 2);
        ImGui::Unindent();
        return edited;
    }
    template<> bool Inspector::inspect<mat3d>(const std::string& name, mat3d& value) {
        ImGui::Text("%s", name.c_str());
        ImGui::Indent();
        bool edited = ImGui::InputScalarN("row #1", ImGuiDataType_Double, &value[0].x(), 3);
        edited |= ImGui::InputScalarN("row #2", ImGuiDataType_Double, &value[1].x(), 3);
        edited |= ImGui::InputScalarN("row #3", ImGuiDataType_Double, &value[2].x(), 3);
        ImGui::Unindent();
        return edited;
    }
    template<> bool Inspector::inspect<mat4d>(const std::string& name, mat4d& value) {
        ImGui::Text("%s", name.c_str());
        ImGui::Indent();
        bool edited = ImGui::InputScalarN("row #1", ImGuiDataType_Double, &value[0].x(), 4);
        edited |= ImGui::InputScalarN("row #2", ImGuiDataType_Double, &value[1].x(), 4);
        edited |= ImGui::InputScalarN("row #3", ImGuiDataType_Double, &value[2].x(), 4);
        edited |= ImGui::InputScalarN("row #4", ImGuiDataType_Double, &value[3].x(), 4);
        ImGui::Unindent();
        return edited;
    }

    template<> bool Inspector::inspect<Inspector::MinMaxNum<mat2d, double>>(const std::string& name, MinMaxNum<mat2d, double>& value) {
        ImGui::Text("%s", name.c_str());
        ImGui::Indent();
        bool edited = ImGui::DragScalarN("row #1", ImGuiDataType_Double, &value.value[0].x(), 2, static_cast<float>(value.speed), &value.min, &value.max);
        edited |= ImGui::DragScalarN("row #2", ImGuiDataType_Double, &value.value[1].x(), 2, static_cast<float>(value.speed), &value.min, &value.max);
        ImGui::Unindent();
        return edited;
    }
    template<> bool Inspector::inspect<Inspector::MinMaxNum<mat3d, double>>(const std::string& name, MinMaxNum<mat3d, double>& value) {
        ImGui::Text("%s", name.c_str());
        ImGui::Indent();
        bool edited = ImGui::DragScalarN("row #1", ImGuiDataType_Double, &value.value[0].x(), 3, static_cast<float>(value.speed), &value.min, &value.max);
        edited |= ImGui::DragScalarN("row #2", ImGuiDataType_Double, &value.value[1].x(), 3, static_cast<float>(value.speed), &value.min, &value.max);
        edited |= ImGui::DragScalarN("row #3", ImGuiDataType_Double, &value.value[2].x(), 3, static_cast<float>(value.speed), &value.min, &value.max);
        ImGui::Unindent();
        return edited;
    }
    template<> bool Inspector::inspect<Inspector::MinMaxNum<mat4d, double>>(const std::string& name, MinMaxNum<mat4d, double>& value) {
        ImGui::Text("%s", name.c_str());
        ImGui::Indent();
        bool edited = ImGui::DragScalarN("row #1", ImGuiDataType_Double, &value.value[0].x(), 4, static_cast<float>(value.speed), &value.min, &value.max);
        edited |= ImGui::DragScalarN("row #2", ImGuiDataType_Double, &value.value[1].x(), 4, static_cast<float>(value.speed), &value.min, &value.max);
        edited |= ImGui::DragScalarN("row #3", ImGuiDataType_Double, &value.value[2].x(), 4, static_cast<float>(value.speed), &value.min, &value.max);
        edited |= ImGui::DragScalarN("row #4", ImGuiDataType_Double, &value.value[3].x(), 4, static_cast<float>(value.speed), &value.min, &value.max);
        ImGui::Unindent();
        return edited;
    }

    bool Inspector::begin_vector(std::string name, bool has_elements) {
        if (vector_depth == 0)
            max_vector_depth = 0;

        while (hovered_vector_names.size() <= vector_depth)
            hovered_vector_names.emplace_back();

        if (!has_elements) {
            ImGui::BeginDisabled();
            ImGui::SmallButton(name.c_str());
            ImGui::EndDisabled();
            return false;
        }

        const auto pos = ImGui::GetCursorScreenPos();

        bool start_window = false;

        ImGui::SmallButton((name + (char*)(u8"\u25B6")).c_str());

        name += std::to_string(pos.y);

        if (ImGui::IsItemHovered()) {
            if (hovered_vector_names[vector_depth].name != name) {
                hovered_vector_names[vector_depth].window_height = 0.0f;
                hovered_vector_names[vector_depth].name = name;
            }
            start_window = true;
        }
        else
            start_window = hovered_vector_names[vector_depth].name == name;

        if (start_window) {
            std::string window_name{};
            for (size_t i = 0; i <= vector_depth; ++i)
                window_name += hovered_vector_names[i].name + ":";

            ImGui::SetNextWindowPos(ImVec2{ImGui::GetWindowPos().x + ImGui::GetWindowSize().x - 1.0f, pos.y});
            ImGui::SetNextWindowSize(ImVec2{-1.0f, hovered_vector_names[vector_depth].window_height});

            ImGui::Begin((window_name).c_str(), nullptr,
                ImGuiWindowFlags_NoTitleBar
                | ImGuiWindowFlags_NoResize
                | ImGuiWindowFlags_NoMove
                | ImGuiWindowFlags_NoSavedSettings
                | ImGuiWindowFlags_NoScrollbar
                | ImGuiWindowFlags_NoDocking
            );
        }

        if (start_window) {
            ++vector_depth;
            max_vector_depth = std::max(max_vector_depth, vector_depth);
        }

        return start_window;
    }

    void Inspector::end_vector() {
        --vector_depth;
        const auto max_height = ImGui::GetCursorPosY();
        hovered_vector_names[vector_depth].window_height = std::lerp(hovered_vector_names[vector_depth].window_height, max_height, 0.2f);
        ImGui::End();

        if (vector_depth == 0 && hovered_vector_names.size() > max_vector_depth) {
            while (hovered_vector_names.size() > max_vector_depth)
                hovered_vector_names.pop_back();
            max_vector_depth = 0;
        }
    }


    Inspector::Inspector() {
        system_name = "Inspector";

        register_type_info<int>("int");
        register_type_info<MinMaxNum<int>>("MinMax int");
        register_type_info<uint32_t>("unsigned int");
        register_type_info<MinMaxNum<uint32_t>>("MinMax unsigned int");
        register_type_info<float>("float");
        register_type_info<MinMaxNum<float>>("MinMax float");
        register_type_info<double>("double");
        register_type_info<MinMaxNum<double>>("MinMax double");

        register_type_info<vec2f>("float vec2");
        register_type_info<MinMaxNum<vec2f, float>>("MinMax float vec2");
        register_type_info<vec3f>("float vec3");
        register_type_info<MinMaxNum<vec3f, float>>("MinMax float vec3");
        register_type_info<vec4f>("float vec4");
        register_type_info<MinMaxNum<vec4f, float>>("MinMax float vec4");

        register_type_info<vec2d>("double vec2");
        register_type_info<MinMaxNum<vec2d, double>>("MinMax double vec2");
        register_type_info<vec3d>("double vec3");
        register_type_info<MinMaxNum<vec3d, double>>("MinMax double vec3");
        register_type_info<vec4d>("double vec4");
        register_type_info<MinMaxNum<vec4d, double>>("MinMax double vec4");

        register_type_info<vec2i32>("int vec2");
        register_type_info<MinMaxNum<vec2i32, int>>("MinMax int vec2");
        register_type_info<vec3i32>("int vec3");
        register_type_info<MinMaxNum<vec3i32, int>>("MinMax int vec3");
        register_type_info<vec4i32>("int vec4");
        register_type_info<MinMaxNum<vec4i32, int>>("MinMax int vec4");

        register_type_info<vec2u32>("unsigned int vec2");
        register_type_info<MinMaxNum<vec2u32, int>>("MinMax unsigned int vec2");
        register_type_info<vec3u32>("unsigned int vec3");
        register_type_info<MinMaxNum<vec3u32, int>>("MinMax unsigned int vec3");
        register_type_info<vec4u32>("unsigned int vec4");
        register_type_info<MinMaxNum<vec4u32, int>>("MinMax unsigned int vec4");

        register_type_info<std::string>("string");

        register_type_info<bool>("checkbox");
    }

    void Inspector::on_begin_play() {
        auto& editor = MagmaEngine{}.systems().get<Editor>();
        editor.add_window<InspectorWindow>();
    }

    void Inspector::update(float delta) {
        (void)delta;
    }

    void Inspector::on_end_play() {
    }
}
