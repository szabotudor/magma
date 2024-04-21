#include "editor.hpp"


namespace mgm {
    void Editor::on_begin_play() {
        Logging{"Editor"}.log("Editor initialized");
    }

    void Editor::in_editor_update(float delta) {
        (void)delta;
    }

    void Editor::on_end_play() {
        Logging{"Editor"}.log("Editor closed");
    }
}
