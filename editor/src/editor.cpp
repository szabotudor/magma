#include "editor.hpp"


namespace mgm {
    MagmaEditor::MagmaEditor() {
    }

    void MagmaEditor::run() {
        while (!window.should_close()) {
            window.update();
        }
    }
} // namespace mgm
