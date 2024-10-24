#include "editor.hpp"
#include "editor_windows/settings.hpp"
#include "inspector.hpp"
#include "engine.hpp"
#include "imgui.h"
#include "editor_windows/file_browser.hpp"


namespace mgm {
    template<>
    bool Inspector::inspect(const std::string&, EditorWindow*& value) {
        ImGui::Checkbox(value->window_name.c_str(), &value->open);
        if (!value->open && value->remove_on_close) {
            MagmaEngine{}.systems().get<Editor>().remove_window(value);
            return true;
        }

        return false;
    }

    bool Editor::draw_palette_options() {
        bool something_selected = false;

        auto& inspector = MagmaEngine{}.systems().get<Inspector>();
        something_selected |= inspector.inspect("Windows", windows);

        if (inspector.begin_window_here("File", true)) {
            if (ImGui::SmallButton("New Project")) {
                add_window<FileBrowser>(
                    true,
                    FileBrowser::Mode::WRITE,
                    FileBrowser::Type::FOLDER,
                    [](const Path& path) {
                        Logging{"Editor"}.log("New project at", path.platform_path());
                    }
                );
            }
            // if (ImGui::SmallButton("Open")) {
            //     add_window<FileBrowser>(true, FileBrowser::Mode::READ, FileBrowser::Type::FILE, [](const Path& path) {
            //         MagmaEngine{}.systems().get<Editor>().add_window<ScriptEditor>(true, path);
            //     });
            //     something_selected = true;
            // }

            // if (ImGui::SmallButton("New")) {
            //     add_window<FileBrowser>(true, FileBrowser::Mode::WRITE, FileBrowser::Type::FILE, [](const Path& path) {
            //         MagmaEngine{}.systems().get<Editor>().add_window<ScriptEditor>(true, path);
            //     }, "new_script");
            //     something_selected = true;
            // }

            if (ImGui::SmallButton("Settings")) {
                add_window<SettingsWindow>(true);
                something_selected = true;
            }

            inspector.end_window_here();
        }

        return something_selected;
    }
}
