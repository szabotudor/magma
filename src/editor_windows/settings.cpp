#include "editor_windows/settings.hpp"

#include "engine.hpp"
#include "imgui.h"


namespace mgm {
    void SettingsWindow::draw_contents() {
        MagmaEngine engine{};

        ImGui::BeginChild("Systems", {}, ImGuiChildFlags_AutoResizeX);
        for (auto& [type_id, system] : engine.systems().systems) {
            if (selected_system == nullptr)
                selected_system = system;

            if (system->should_appear_in_settings_window)
                if (ImGui::Selectable(system->system_name.c_str(), selected_system == system))
                    selected_system = system;
        }
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("Settings", {}, ImGuiChildFlags_Border);
        if (selected_system != nullptr)
            selected_system->draw_settings_window_contents();

        ImGui::EndChild();
    }
} // namespace mgm
