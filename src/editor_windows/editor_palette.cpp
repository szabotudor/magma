#include "editor.hpp"
#include "inspector.hpp"
#include "engine.hpp"
#include "imgui.h"
#include "editor_windows/script_editor.hpp"
#include "editor_windows/file_browser.hpp"


namespace mgm {
    template<>
    bool Inspector::inspect(const std::string&, EditorWindow*& value) {
        ImGui::Checkbox(value->name.c_str(), &value->open);
        if (!value->open && value->remove_on_close) {
            MagmaEngine{}.systems().get<Editor>().remove_window(value);
            return true;
        }

        return false;
    }

    template<>
    bool Inspector::inspect(const std::string&, Path& value) {
        if (ImGui::SmallButton((value - Path::assets).data.c_str())) {
            MagmaEngine{}.systems().get<Editor>().add_window<ScriptEditor>(true, value);
            return true;
        }
        return false;
    }

    bool Editor::palette_options() {
        bool something_selected = false;

        auto& inspector = MagmaEngine{}.systems().get<Inspector>();
        something_selected |= inspector.inspect("Windows", windows);

        if (inspector.begin_window_here("File", true)) {
            if (ImGui::SmallButton("Open")) {
                add_window<FileBrowser>(true, FileBrowser::Mode::READ);
                something_selected = true;
            }

            if (ImGui::SmallButton("New")) {
                add_window<FileBrowser>(true, FileBrowser::Mode::WRITE, "new_script");
                something_selected = true;
            }

            inspector.end_window_here();
        }

        return something_selected;
    }
}
