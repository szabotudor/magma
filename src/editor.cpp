#include "editor.hpp"
#include "engine.hpp"
#include "file.hpp"
#include "helpers.hpp"
#include "imgui_impl_mgmgpu.h"
#include "json.hpp"
#include "logging.hpp"
#include "mgmwin.hpp"
#include "imgui.h"
#include "notifications.hpp"
#include "systems.hpp"
#include "input.hpp"


namespace mgm {
    void EditorWindow::draw_window() {
        if (!open)
            return;

        bool prev_open = open;
        ImGui::BeginResizeable(window_name.c_str(), &open);

        if (!open && prev_open && remove_on_close) {
            ImGui::End();
            MagmaEngine{}.systems().get<Editor>().remove_window(this);
            return;
        }

        draw_contents();
        ImGui::End();
    }


    Editor::Editor() {
        system_name = "Editor";

        ImGuiIO& io = ImGui::GetIO();
        static const ImWchar ranges[] = {
            0x0020, 0x00FF, // Basic Latin + Latin Supplement
            0x25A0, 0x25FF, // Geometric Shapes
            0,
        };
        font_id = io.Fonts->AddFontFromFileTTF(Path("resources://fonts/Hack-Regular.ttf").platform_path().c_str(), 20.0f, nullptr, ranges);
        
        auto engine = MagmaEngine{};

        Logging{"Editor"}.log("Editor initialized");

        engine.input().register_input_action("open_palette", MgmWindow::InputInterface::Key_SPACE, {MgmWindow::InputInterface::Key_CTRL});
        engine.input().register_input_action("escape", MgmWindow::InputInterface::Key_ESC);
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

    void Editor::update(float delta) {
        time_since_last_save += delta;
        if (time_since_last_save > 5.0f) {
            save_current_project();
            time_since_last_save = 0.0f;
        }


        ImGui::PushFont((ImFont*)font_id);

        for (const auto window : windows)
            window->draw_window();

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
                static_cast<int>((engine.window().get_input_interface(MgmWindow::InputInterface::Mouse_POS_X) + 1.0f) * 0.5f * (float)engine.window().get_size().x()),
                static_cast<int>((engine.window().get_input_interface(MgmWindow::InputInterface::Mouse_POS_Y) + 1.0f) * 0.5f * (float)engine.window().get_size().y())
            };
            if (palette_open) {
                ImGui::SetNextWindowPos(ImVec2{static_cast<float>(mouse_pos.x() + 16), static_cast<float>(mouse_pos.y())});
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
        
        if (project.type() != JObject::Type::OBJECT || !project.has("placeholder")) {
            const auto message = "No valid project to load at: \"" + project_path.platform_path() + "\"";
            engine.notifications().push(message, {1.0f, 0.2f, 0.2f, 1.0f});
            Logging{"Editor"}.error(message);
            return;
        }

        unload_project();
        Path::setup_project_dirs(project_path.platform_path(), (project_path / "assets").platform_path(), (project_path / "data").platform_path());

        if (engine.file_io().exists("project://.mgm")) {
            if (engine.file_io().exists("project://.mgm/.layout")) {
                const JObject layout = engine.file_io().read_text("project://.mgm/.layout");
                const auto& window = layout["window"];
                uint32_t size_x = (uint32_t)window["size_x"];
                uint32_t size_y = (uint32_t)window["size_y"];
                int32_t pos_x = (int32_t)window["pos_x"];
                int32_t pos_y = (int32_t)window["pos_y"];

                engine.window().set_size({size_x, size_y});
                engine.window().set_position({pos_x, pos_y});
            }
        }
        
        const auto message = "Loaded project from: \"" + project_path.platform_path() + "\"";
        engine.notifications().push(message);
        Logging{"Editor"}.log(message);
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
        window["size_x"] = size.x();
        window["size_y"] = size.y();
        window["pos_x"] = engine.window().get_position().x();
        window["pos_y"] = engine.window().get_position().y();

        engine.file_io().write_text("project://.mgm/.layout", layout);
    }

    void Editor::initialize_project(const Path& project_path) {
        MagmaEngine engine{};

        if (engine.file_io().exists(project_path / ".magma")) {
            const auto message = "Project already exists at: \"" + project_path.platform_path() + "\"";
            engine.notifications().push(message, {1.0f, 0.2f, 0.2f, 1.0f});
            Logging{"Editor"}.error(message);
        }

        JObject project{};
        project["placeholder"] = true;

        engine.file_io().write_text(project_path / ".magma", project);
        
        const auto message = "Created new project at: \"" + project_path.platform_path() + "\"";
        engine.notifications().push(message);
        Logging{"Editor"}.log(message);

        load_project(project_path);
    }

    void Editor::unload_project() {
        Path::setup_project_dirs(FileIO::exe_dir().platform_path(), (FileIO::exe_dir() / "assets").platform_path(), (FileIO::exe_dir() / "data").platform_path());
    }

    Editor::~Editor() {
        save_current_project();
        for (const auto window : windows)
            delete window;
        Logging{"Editor"}.log("Editor closed");
    }
} // namespace mgm
