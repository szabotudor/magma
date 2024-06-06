#include "editor_windows/settings.hpp"
#include "engine.hpp"
#include "imgui.h"


namespace mgm {
    void EditorSettings::draw_contents() {
        MagmaEngine engine{};

        ImGui::BeginChild("Systems", {}, ImGuiChildFlags_AutoResizeX);
        for (auto& [type_id, system] : engine.systems().systems) {
            if (selected_system == 0)
                selected_system = type_id;

            const auto it = RegisterSubsectionFunction::get_subsections_map().find(type_id);
            if (it == RegisterSubsectionFunction::get_subsections_map().end()) {
                ImGui::TreeNodeEx(system->system_name.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
                continue;
            }

            if (ImGui::TreeNodeEx(system->system_name.c_str(), ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen)) {
                if (it != RegisterSubsectionFunction::get_subsections_map().end()) {
                    for (auto& [name, func] : it->second) {
                        ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
                        if (ImGui::IsItemClicked())
                            func(system);
                    }
                }
                ImGui::TreePop();
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("Settings", {}, ImGuiChildFlags_Border);
        ImGui::EndChild();
    }
}
