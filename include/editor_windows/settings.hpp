#pragma once
#include "systems/editor.hpp"
#include "systems.hpp"


namespace mgm {
    class SettingsManager : public System {
        public:
        SettingsManager();
    };

    class SettingsWindow : public EditorWindow {
        System* selected_system = nullptr;

    public:
        SettingsWindow() {
            window_name = "Settings";
        }

        void draw_contents() override;
    };
}
