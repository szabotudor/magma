#include "systems/editor.hpp"
#include "editor_windows/file_browser.hpp"
#include "engine.hpp"
#include "file.hpp"
#include "helpers.hpp"
#include "json.hpp"
#include "logging.hpp"
#include "mgmwin.hpp"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "systems/notifications.hpp"
#include "systems.hpp"
#include "systems/input.hpp"
#include "tools/base64.hpp"


namespace mgm {
    void EditorWindow::draw_window() {
        if (!open)
            return;

        ImGui::Begin(window_name.c_str(), &open);

        draw_contents();
        ImGui::End();
    }


    Editor::Editor() {
        system_name = "Editor";

        ImGuiIO& io = ImGui::GetIO();
        static const ImWchar ranges[] = {
            0x0020, 0x00FF, // Basic Latin + Latin Supplement
            0x25A0, 0x25FF, // Geometric Shapes
            0x2700, 0x27BF, // Dingbats
            0
        };
        font_id = io.Fonts->AddFontFromFileTTF(Path("resources://fonts/Hack-Regular.ttf").platform_path().c_str(), 20.0f, nullptr, ranges);
        
        auto engine = MagmaEngine{};

        Logging{"Editor"}.log("Editor initialized");

        engine.input().register_input_action("open_palette", MgmWindow::InputInterface::Key_SPACE, {MgmWindow::InputInterface::Key_CTRL});
        engine.input().register_input_action("escape", MgmWindow::InputInterface::Key_ESC);

        if (engine.file_io().exists("data://recents.json")) {
            const JObject recents_json = engine.file_io().read_text("data://recents.json");
            const auto recents = recents_json["recents"];
            for (const auto& [key, val] : recents) {
                recent_project_dirs.emplace_back(val);
            }
        }
    }

    bool Editor::begin_window_here(std::string name, bool has_elements) {
        if (vector_depth == 0)
            max_vector_depth = 0;

        while (hovered_vector_names.size() <= vector_depth)
            hovered_vector_names.emplace_back();

        if (!has_elements) {
            ImGui::BeginDisabled();
            ImGui::SmallButton(name.c_str());
            ImGui::EndDisabled();
            return false;
        }

        const auto pos = ImGui::GetCursorScreenPos();

        bool start_window = false;

        ImGui::SmallButton((name + (const char*)(u8"\u25B6")).c_str());

        name += std::to_string(pos.y);

        if (ImGui::IsItemHovered()) {
            if (hovered_vector_names[vector_depth].name != name) {
                hovered_vector_names[vector_depth].window_height = 0.0f;
                hovered_vector_names[vector_depth].name = name;
            }
            start_window = true;
        }
        else
            start_window = hovered_vector_names[vector_depth].name == name;

        if (start_window) {
            std::string window_name{};
            for (size_t i = 0; i <= vector_depth; ++i)
                window_name += hovered_vector_names[i].name + ":";

            ImGui::SetNextWindowPos(ImVec2{ImGui::GetWindowPos().x + ImGui::GetWindowSize().x - 1.0f, pos.y});
            ImGui::SetNextWindowSize(ImVec2{-1.0f, hovered_vector_names[vector_depth].window_height});

            ImGui::Begin((window_name).c_str(), nullptr,
                ImGuiWindowFlags_NoTitleBar
                | ImGuiWindowFlags_NoResize
                | ImGuiWindowFlags_NoMove
                | ImGuiWindowFlags_NoSavedSettings
                | ImGuiWindowFlags_NoScrollbar
                | ImGuiWindowFlags_NoDocking
            );
        }

        if (start_window) {
            ++vector_depth;
            max_vector_depth = std::max(max_vector_depth, vector_depth);
        }

        return start_window;
    }

    void Editor::end_window_here() {
        if (vector_depth == 0) {
            Logging{"Inspector"}.error("end_window_here called without a matching begin_window_here, or called when begin_window_here returned false.\n\tSkipping end_window_here call.");
            return;
        }
        --vector_depth;
        const auto max_height = ImGui::GetCursorPosY();
        hovered_vector_names[vector_depth].window_height = std::lerp_with_delta(hovered_vector_names[vector_depth].window_height, max_height, 50.0f, MagmaEngine{}.delta_time());
        ImGui::End();

        if (vector_depth == 0 && hovered_vector_names.size() > max_vector_depth) {
            while (hovered_vector_names.size() > max_vector_depth)
                hovered_vector_names.pop_back();
            max_vector_depth = 0;
        }
    }

    bool Editor::is_running() const {
        return false; // TODO: Implement running the game in the editor, for now just return false
    }
    
    void Editor::draw_settings_window_contents() {
        static std::string section_name = "";

        ImGui::BeginChild("Sections", {0, 0}, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_Border);

        if (ImGui::Selectable("Project", section_name == "Project"))
            section_name = "Project";
        if (ImGui::Selectable("Script Editor", section_name == "Script Editor"))
            section_name = "Script Editor";

        ImGui::EndChild();

        if (section_name == "")
            return;

        ImGui::SameLine();

        ImGui::BeginChild(section_name.c_str(), {0, 0}, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_Border);

        if (section_name == "Project") {
            if (!is_a_project_loaded()) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                ImGui::Text("No Project Loaded");
                ImGui::PopStyleColor();
                ImGui::EndChild();
                return;
            }

            if (ImGui::InputText("Project Name", &project_name))
                save_current_project();

            ImGui::Text("Main Scene File | ");
            ImGui::SameLine();
            ImGui::Text("%s | ", main_scene_path.data.c_str());
            ImGui::SameLine();

            if (ImGui::Button("Browse")) {
                add_window<FileBrowser>(true, FileBrowser::Args{
                    .mode = FileBrowser::Mode::READ,
                    .type = FileBrowser::Type::FILE,
                    .callback = [&](const Path& path) {
                        main_scene_path = path;
                        save_current_project();
                    },
                    .allow_paths_outside_project = false,
                    .default_file_name = "New Scene",
                    .default_file_extension = ".lua",
                });
            }
        }

        else if (section_name == "Script Editor") {
            ImGui::Text("Script Editor Settings");
        }

        ImGui::EndChild();
    }

    void Editor::update(float delta) {
        time_since_last_save += delta;
        if (time_since_last_save > 5.0f) {
            save_current_project();
            time_since_last_save = 0.0f;
        }


        ImGui::PushFont((ImFont*)font_id);

        for (size_t i = 0; i < windows.size(); ++i) {
            auto& window = windows[i];
            if (!window->open && window->remove_on_close) {
                remove_window(window);
                --i;
                continue;
            }
            window->draw_window();
        }

        auto engine = MagmaEngine{};

        if (!project_initialized)
            engine.notifications().push("Welcome to MagmaEngine. Press 'ctrl+space' to open the editor palette and start editing your project.");

        for (const auto& [type_id, system] : engine.systems().systems)
            system->in_editor_update(delta);

        vec2i32 mouse_pos{-1, -1};


        if (engine.input().is_action_just_pressed("open_palette") || (palette_open && engine.input().is_action_just_pressed("escape"))) {
            palette_open = !palette_open;
            if (!project_initialized)
                project_initialized = true;
            mouse_pos = {
                static_cast<int>((engine.window().get_input_interface(MgmWindow::InputInterface::Mouse_POS_X) + 1.0f) * 0.5f * (float)engine.window().get_size().x),
                static_cast<int>((engine.window().get_input_interface(MgmWindow::InputInterface::Mouse_POS_Y) + 1.0f) * 0.5f * (float)engine.window().get_size().y)
            };
            if (palette_open) {
                ImGui::SetNextWindowPos(ImVec2{static_cast<float>(mouse_pos.x + 16), static_cast<float>(mouse_pos.y)});
                ImGui::SetNextWindowSize(ImVec2{-1.0f, 1.0f});
            }
        }

        if (!palette_open) {
            palette_window_height = std::lerp_with_delta(palette_window_height, 0.0f, 50.0f, delta);
            if (palette_window_height < ImGui::GetTextLineHeight()) {
                ImGui::PopFont();
                return;
            }
        }

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.0f, 0.0f, 0.0f, 0.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{1.0f, 1.0f, 1.0f, 0.1f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{1.0f, 1.0f, 1.0f, 0.2f});

        ImGui::SetNextWindowSize(ImVec2{-1.0f, palette_window_height});

        ImGui::Begin("Palette", nullptr,
            ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoSavedSettings
            | ImGuiWindowFlags_NoScrollbar
            | ImGuiWindowFlags_NoDocking
        );

        ImGui::Text("Palette");
        ImGui::Separator();
        for (const auto& [type_id, system] : engine.systems().systems) {
            if (system->draw_palette_options())
                palette_open = false;
        }

        ImGui::PopStyleColor(3);

        if (palette_open) {
            const auto max_height = ImGui::GetCursorPosY();
            palette_window_height = std::lerp_with_delta(palette_window_height, max_height, 50.0f, delta);
        }

        ImGui::End();
        ImGui::PopFont();
    }

    bool Editor::location_contains_project(const Path& location) {
        MagmaEngine engine{};

        if (!engine.file_io().exists(location))
            return false;

        const auto files = engine.file_io().list_files(location);

        for (const auto& file : files)
            if (file.file_name() == ".magma")
                return true;

        return false;
    }

    Path Editor::currently_loaded_project() {
        return Path::project_dir;
    }

    bool Editor::is_a_project_loaded() {
        return currently_loaded_project().platform_path() != FileIO::exe_dir().data;
    }

    void Editor::load_project(const Path& project_path) {
        MagmaEngine engine{};

        const auto project = JObject{engine.file_io().read_text(project_path / ".magma")};

        if (project.type() != JObject::Type::OBJECT || !project.has("name")) {
            const auto message = "No valid project to load at: \"" + project_path.platform_path() + "\"";
            engine.notifications().push(message, {1.0f, 0.2f, 0.2f, 1.0f});
            Logging{"Editor"}.error(message);
            return;
        }

        unload_project();

        // Read the list of recent projects
        {
            auto& editor = engine.editor();
            std::vector<JObject> recents_json_list{};

            if (engine.file_io().exists("data://recents.json")) {
                const JObject recents_json = engine.file_io().read_text("data://recents.json");
                recents_json_list = recents_json["recents"];
            }
            
            editor.recent_project_dirs.clear();
            for (const auto& r : recents_json_list)
                editor.recent_project_dirs.emplace_back(r);

            const auto it = std::find(editor.recent_project_dirs.begin(), editor.recent_project_dirs.end(), project_path);
            if (it != editor.recent_project_dirs.end())
                editor.recent_project_dirs.erase(it);

            editor.recent_project_dirs.insert(editor.recent_project_dirs.begin(), project_path);

            if (editor.recent_project_dirs.size() > 10)
                editor.recent_project_dirs.pop_back();

            recents_json_list.clear();
            for (const auto& r : editor.recent_project_dirs)
                recents_json_list.emplace_back(JObject{r.platform_path()});
            JObject recents_json{};
            recents_json["recents"] = recents_json_list;
            engine.file_io().write_text("data://recents.json", recents_json);
        }
        
        if (!engine.file_io().exists(project_path / "assets"))
            engine.file_io().create_folder(project_path / "assets");
        if (!engine.file_io().exists(project_path / "data"))
            engine.file_io().create_folder(project_path / "data");

        Path::setup_project_dirs(project_path.platform_path(), (project_path / "assets").platform_path(), (project_path / "data").platform_path());

        if (engine.file_io().exists("project://.mgm")) {
            if (engine.file_io().exists("project://.mgm/.layout")) {
                const JObject layout = engine.file_io().read_text("project://.mgm/.layout");
                const auto& window = layout["window"];
                if (!window.empty()) {
                    uint32_t size_x = (uint32_t)window["size_x"];
                    uint32_t size_y = (uint32_t)window["size_y"];
                    int32_t pos_x = (int32_t)window["pos_x"];
                    int32_t pos_y = (int32_t)window["pos_y"];

                    engine.window().set_size({size_x, size_y});
                    engine.window().set_position({pos_x, pos_y});
                }

                if (layout.has("imgui")) {
                    const auto& imgui = layout["imgui"];
                    if (!imgui.empty()) {
                        const std::string& data = imgui["ini_data"];
                        const auto decoded_data = base64::decode_into<std::string>(data);
                        ImGui::LoadIniSettingsFromMemory(decoded_data.c_str(), decoded_data.size());
                    }
                }
            }
        }
        
        const auto message = "Loaded project from: \"" + project_path.platform_path() + "\"";
        engine.notifications().push(message);
        Logging{"Editor"}.log(message);

        JObject loading_project = engine.file_io().read_text("project://.magma");
        auto& editor = engine.editor();
        editor.project_name = loading_project["name"];
        editor.main_scene_path.data = loading_project["main_scene_path"];
    }

    void Editor::save_current_project() {
        if (!is_a_project_loaded())
            return;
        
        MagmaEngine engine{};

        if (!engine.file_io().exists("project://.mgm"))
            engine.file_io().create_folder("project://.mgm");
        
        JObject layout{};
        const auto size = engine.window().get_size();
        auto& window = layout["window"];
        window["size_x"] = size.x;
        window["size_y"] = size.y;
        window["pos_x"] = engine.window().get_position().x;
        window["pos_y"] = engine.window().get_position().y;

        auto& imgui = layout["imgui"];
        size_t imgui_settings_dump_size{};
        const auto imgui_settings = ImGui::SaveIniSettingsToMemory(&imgui_settings_dump_size);
        imgui["ini_data"] = base64::encode_into<std::string>(imgui_settings, imgui_settings + imgui_settings_dump_size);

        engine.file_io().write_text("project://.mgm/.layout", layout);

        JObject project{};
        project["name"] = engine.editor().project_name;
        project["main_scene_path"] = engine.editor().main_scene_path.data;

        engine.file_io().write_text("project://.magma", project);
    }

    void Editor::initialize_project(const Path& project_path) {
        MagmaEngine engine{};

        if (engine.file_io().exists(project_path / ".magma")) {
            const auto message = "Project already exists at: \"" + project_path.platform_path() + "\"";
            engine.notifications().push(message, {1.0f, 0.2f, 0.2f, 1.0f});
            Logging{"Editor"}.error(message);
        }

        JObject project{};
        project["name"] = "New Project";
        project["main_scene_path"] = "";

        engine.file_io().write_text(project_path / ".magma", project);
        
        const auto message = "Created new project at: \"" + project_path.platform_path() + "\"";
        engine.notifications().push(message);
        Logging{"Editor"}.log(message);
        
        engine.file_io().create_folder(project_path / "assets");
        engine.file_io().create_folder(project_path / "data");

        load_project(project_path);
    }

    void Editor::unload_project() {
        auto& editor = MagmaEngine{}.editor();
        for (const auto window : editor.windows)
            delete window;
        editor.windows.clear();

        Path::setup_project_dirs(FileIO::exe_dir().platform_path(), (FileIO::exe_dir() / "assets").platform_path(), (FileIO::exe_dir() / "data").platform_path());
        editor.project_name = "";
        editor.main_scene_path = "";
    }

    Editor::~Editor() {
        save_current_project();
        for (const auto window : windows)
            delete window;
        Logging{"Editor"}.log("Editor closed");
    }
} // namespace mgm
