#include "editor_windows/file_browser.hpp"
#include "editor_windows/script_editor.hpp"
#include "engine.hpp"
#include "imgui.h"
#include "imgui_stdlib.h"
#include <cstddef>


namespace mgm {
    void FileBrowser::draw_contents() {
        MagmaEngine engine{};

        if (mode == Mode::WRITE) {
            ImGui::InputText("Name", &file_name);

            if (ImGui::Button("Create File")) {
                engine.file_io().write_binary(file_path / file_name, default_contents);
                open = false;
                engine.systems().get<Editor>().add_window<ScriptEditor>(true, file_path / file_name);
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Create a new file with the name specified in the Name field");
        }

        ImGui::SameLine();
        if (ImGui::Button("Create Folder")) {
            engine.file_io().create_folder(file_path / file_name);
            file_path = file_path / file_name;
            selected_file = (size_t)-1;
            file_name.clear();
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Create a new folder with the name specified in the Name field");

        ImGui::SameLine();
        if ((ImGui::Button("Delete Selected") || ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete))) && selected_file != (size_t)-1) {
            if (selected_file != (size_t)-1 && selected_file != (size_t)-2) {
                if (selected_file < folders_here.size())
                    engine.file_io().delete_file(folders_here[selected_file]);
                else
                    engine.file_io().delete_file(files_here[selected_file - folders_here.size()]);
                selected_file = (size_t)-1;
                folders_here = engine.file_io().list_folders(file_path);
                files_here = engine.file_io().list_files(file_path);
            }
        }
        
        ImGui::Separator();

        if (mode == Mode::READ) {
            if (selected_file == (size_t)-1)
                ImGui::Text("Open a file");
            else {
                if (selected_file < folders_here.size())
                    ImGui::Text("Open Folder: %s", folders_here[selected_file].file_name().c_str());
                else if (selected_file != (size_t)-1 && selected_file != (size_t)-2) {
                    const auto files = engine.file_io().list_files(file_path);
                    ImGui::Text("Open File: %s", files[selected_file - folders_here.size()].file_name().c_str());
                }
                else
                    ImGui::Text("No file selected");
            }
            ImGui::Separator();
        }

        ImGui::Text("Path: %s", file_path.data.c_str());
        ImGui::Separator();

        if (ImGui::Selectable("..##_folders", selected_file == (size_t)-1)) {
            if (selected_file == (size_t)-1) {
                const auto file_back = file_path.back();
                if (file_back.validity() != Path::Validity::VALID)
                    return;
                file_path = file_back;
                file_name = "New File";
                selected_file = (size_t)-1;
                folders_here = engine.file_io().list_folders(file_path);
                files_here = engine.file_io().list_files(file_path);
                return;
            }
            selected_file = (size_t)-1;
            return;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Go up one folder");
        
        if (type == Type::FOLDER) {
            if (ImGui::Selectable("<< Use This Folder >>", selected_file == (size_t)-2)) {
                if (selected_file == (size_t)-2) {
                    if (callback)
                        callback(file_path);
                    open = false;
                    selected_file = (size_t)-1;
                    return;
                }
                selected_file = (size_t)-2;
                return;
            }
        }

        size_t i = 0;
        for (const auto& sub_folder : folders_here) {
            if (ImGui::Selectable((sub_folder.file_name() + "##_folders").c_str(), i == selected_file)) {
                if (i == selected_file) {
                    file_path = sub_folder;
                    file_name = "New File";
                    selected_file = (size_t)-1;
                    folders_here = engine.file_io().list_folders(file_path);
                    files_here = engine.file_io().list_files(file_path);
                    return;
                }
                else {
                    file_name = sub_folder.file_name();
                    selected_file = i;
                }
            }
            ++i;
        }

        if (type == Type::FILE)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.0f, 0.7f, 0.7f, 1.0f});
        else
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));

        for (const auto& file : files_here) {
            if (ImGui::Selectable((file.file_name() + "##_files").c_str(), i == selected_file)) {
                if (i == selected_file) {
                    file_name = file.file_name();
                    if (mode == Mode::WRITE)
                        engine.file_io().write_binary(file_path / (file_name), default_contents);
                    open = false;
                    if (callback)
                        callback(file_path / file_name);
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
                ImGui::SetTooltip("Replace this file with an empty one");
            ++i;
        }
        ImGui::PopStyleColor();
    }
} // namespace mgm
