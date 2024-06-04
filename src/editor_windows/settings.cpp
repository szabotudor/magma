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

            if (type_id == selected_system)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.2f, 0.2f, 1.0f});

            ImGui::SmallButton(system->system_name.c_str());

            if (type_id == selected_system)
                ImGui::PopStyleColor();
            if (ImGui::IsItemClicked())
                selected_system = type_id;
        }
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("Settings", {}, ImGuiChildFlags_Border);
        engine.systems().systems[selected_system]->draw_settings_window_contents();
        ImGui::EndChild();
    }
}
