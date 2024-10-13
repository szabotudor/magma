#include "editor_windows/settings.hpp"

#include "engine.hpp"
#include "expose_api.hpp"
#include "imgui.h"


namespace mgm {
    void SettingsWindow::draw_contents() {
        MagmaEngine engine{};

        ImGui::BeginChild("Systems", {}, ImGuiChildFlags_AutoResizeX);
        for (auto& [type_id, system] : engine.systems().systems) {
            if (selected_system == nullptr)
                selected_system = system;

            const auto exposed_system = dynamic_cast<ExposeApiRuntime*>(system);
            if (exposed_system == nullptr)
                continue;

            if (ImGui::TreeNodeEx(system->system_name.c_str(), ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen)) {
                for (auto& [name, func] : exposed_system->get_all_members()) {
                    if (func.is_variable())
                        continue;
                    ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
                    if (ImGui::IsItemClicked()) {
                        draw_settings_func = [func]() mutable {func.call<void>();};
                        selected_system = system;
                    }
                }
                ImGui::TreePop();
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("Settings", {}, ImGuiChildFlags_Border);
        if (draw_settings_func)
            draw_settings_func();
        ImGui::EndChild();
    }
}
