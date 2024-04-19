#include "editor.hpp"


namespace mgm {
    void Editor::init() {
        Logging{"Editor"}.log("Editor initialized");
    }

    void Editor::in_editor_update(float delta) {
        (void)delta;
        Logging{"Editor"}.log("Editor updated");
    }

    void Editor::close() {
        Logging{"Editor"}.log("Editor closed");
    }
}
