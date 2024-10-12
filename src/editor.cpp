#include "editor.hpp"
#include "engine.hpp"
#include "helpers.hpp"
#include "imgui_impl_mgmgpu.h"
#include "inspector.hpp"
#include "json.hpp"
#include "logging.hpp"
#include "mgmwin.hpp"
#include "imgui.h"
#include "notifications.hpp"
#include "file.hpp"
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
        font_id = io.Fonts->AddFontFromFileTTF("resources/fonts/Hack-Regular.ttf", 20.0f, nullptr, ranges);
        
        auto engine = MagmaEngine{};
        engine.systems().create<Inspector>();

        Logging{"Editor"}.log("Editor initialized");

        engine.input().register_input_action("open_palette", MgmWindow::InputInterface::Key_SPACE, {MgmWindow::InputInterface::Key_CTRL});
        engine.input().register_input_action("escape", MgmWindow::InputInterface::Key_ESC);

        if (engine.file_io().exists("exe://project.json")) {
            const auto project_properties_file = engine.file_io().read_text("exe://project.json");
            const JObject project_properties {project_properties_file};

            project_initialized = (bool)project_properties["initialized"];
        }
    }

    void Editor::update(float delta) {
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

    Editor::~Editor() {
        for (const auto window : windows)
            delete window;
        Logging{"Editor"}.log("Editor closed");

        auto engine = MagmaEngine{};

        JObject project_properties{};
        project_properties["initialized"] = project_initialized;

        engine.file_io().write_text("exe://project.json", project_properties);
    }
} // namespace mgm
