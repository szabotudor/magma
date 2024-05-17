#include "editor_windows/file_browser.hpp"
#include "editor_windows/script_editor.hpp"
#include "engine.hpp"
#include "imgui.h"
#include "imgui_stdlib.h"


namespace mgm {
    void FileBrowser::draw_contents() {
        MagmaEngine engine{};

        if (mode == Mode::WRITE) {
            ImGui::InputText("Name", &file_name);

            if (ImGui::Button("Create")) {
                engine.file_io().write_binary(file_path / (file_name), default_contents);
                open = false;
                engine.systems().get<Editor>().add_window<ScriptEditor>(true, file_path / file_name);
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Create a new script with the name specified in the Name field");

            ImGui::SameLine();
            if (ImGui::Button("New Folder")) {
                engine.file_io().create_folder(file_path / file_name);
                file_name.clear();
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Create a new folder with the name specified in the Name field");

            ImGui::Separator();
        }
        else if (mode == Mode::READ) {
            if (selected_file == (size_t)-1)
                ImGui::Text("Open a file");
            else {
                const auto folders = engine.file_io().list_folders(file_path);
                if (selected_file < folders.size())
                    ImGui::Text("Open Folder: %s", folders[selected_file].file_name().c_str());
                else {
                    const auto files = engine.file_io().list_files(file_path);
                    ImGui::Text("Open File: %s", files[selected_file - folders.size()].file_name().c_str());
                }
            }
            ImGui::Separator();
        }

        ImGui::Text("Path: %s", file_path.platform_path().c_str());
        ImGui::Separator();

        if (ImGui::Selectable("..", false)) {
            file_path = file_path.back();
            selected_file = (size_t)-1;
            return;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Go up one folder");

        size_t i = 0;
        for (const auto& sub_folder : engine.file_io().list_folders(file_path)) {
            if (ImGui::Selectable(sub_folder.file_name().c_str(), i == selected_file)) {
                if (i == selected_file) {
                    file_path = sub_folder;
                    file_name = "New File";
                    selected_file = (size_t)-1;
                    return;
                }
                else {
                    file_name = sub_folder.file_name();
                    selected_file = i;
                }
            }
            ++i;
        }
        
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.0f, 0.7f, 0.7f, 1.0f});
        for (const auto& file : engine.file_io().list_files(file_path)) {
            if (ImGui::Selectable(file.file_name().c_str(), i == selected_file)) {
                if (i == selected_file) {
                    file_name = file.file_name();
                    if (mode == Mode::WRITE)
                        engine.file_io().write_binary(file_path / (file_name), default_contents);
                    open = false;
                    engine.systems().get<Editor>().add_window<ScriptEditor>(true, file_path / file_name);
                    selected_file = (size_t)-1;
                    ImGui::PopStyleColor();
                    return;
                }
                else {
                    file_name = file.file_name();
                    selected_file = i;
                }
            }
            if (mode == Mode::WRITE && ImGui::IsItemHovered())
                ImGui::SetTooltip("Replace this file with new default contents for");
            ++i;
        }
        ImGui::PopStyleColor();
    }
} // namespace mgm
