#pragma once
#include "editor.hpp"


namespace mgm {
    class EditorSettings : public EditorWindow {
        size_t selected_system = 0;

    public:
        EditorSettings() {
            name = "Settings";
        }

        void draw_contents() override;
    };
}
