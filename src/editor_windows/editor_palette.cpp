#include "systems/editor.hpp"
#include "editor_windows/settings.hpp"
#include "engine.hpp"
#include "imgui.h"
#include "editor_windows/file_browser.hpp"


namespace mgm {
    bool Editor::draw_palette_options() {
        bool something_selected = false;

        for (const auto& window : windows) {
            ImGui::Checkbox(window->window_name.c_str(), &window->open);
            if (!window->open && window->remove_on_close) {
                remove_window(window);
                break;
            }
        }

        if (begin_window_here("File", true)) {
            if (ImGui::SmallButton("New Project")) {
                palette_open = false;
                add_window<FileBrowser>(
                    true,
                    FileBrowser::Args {
                        .mode = FileBrowser::Mode::WRITE,
                        .type = FileBrowser::Type::FOLDER,
                        .callback = [](const Path path) {
                            if (location_contains_project(path)) {
                                Logging{"Editor"}.error("A project already exists in this location: \"", path.platform_path(), "\"");
                                return;
                            }
                            initialize_project(path);
                        },
                        .allow_paths_outside_project = true
                    }
                );
            }

            if (ImGui::SmallButton("Open Project")) {
                palette_open = false;
                add_window<FileBrowser>(
                    true,
                    FileBrowser::Args {
                        .mode = FileBrowser::Mode::READ,
                        .type = FileBrowser::Type::FOLDER,
                        .callback = [](const Path path) {
                            if (!location_contains_project(path)) {
                                Logging{"Editor"}.error("No project to open at location: \"", path.platform_path(), "\"");
                                return;
                            }
                            load_project(path);
                        }
                    }
                );
            }

            if (begin_window_here("Recent Projects", !recent_project_dirs.empty())) {
                for (const auto& path : recent_project_dirs) {
                    if (ImGui::SmallButton(path.file_name().c_str())) {
                        palette_open = false;
                        if (location_contains_project(path)) {
                            // Workaround because the reference might be invalidated during the load_project call
                            const auto path_copy = path;
                            load_project(path_copy);
                        }
                    }
                }
                end_window_here();
            }

            bool cant_unload = false;
            if (currently_loaded_project().empty() || currently_loaded_project().data == FileIO::exe_dir().data) {
                ImGui::BeginDisabled();
                cant_unload = true;
            }

            if (ImGui::SmallButton("Close Current Project")) {
                palette_open = false;
                unload_project();
            }

            if (cant_unload)
                ImGui::EndDisabled();

            ImGui::Separator();

            if (ImGui::SmallButton("Settings")) {
                add_window<SettingsWindow>(true);
                something_selected = true;
            }

            end_window_here();
        }

        return something_selected;
    }
}
