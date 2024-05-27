#include "editor.hpp"
#include "engine.hpp"
#include "helper_math.hpp"
#include "inspector.hpp"
#include "logging.hpp"
#include "mgmwin.hpp"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "editor_windows/script_editor.hpp"
#include "notifications.hpp"
#include "systems.hpp"
#include <cmath>


namespace mgm {
    void EditorWindow::draw_window() {
        if (!open)
            return;

        bool prev_open = open;
        ImGui::Begin(name.c_str(), &open);

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
    }

    void Editor::on_begin_play() {
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

        if (!engine.file_io().exists(Path::assets))
            engine.file_io().create_folder(Path::assets);

        engine.notifications().push("Welcome to MagmaEngine. Press 'ctrl+space' to open the editor palette.");
    }

    void Editor::update(float delta) {
        ImGui::PushFont((ImFont*)font_id);

        for (const auto window : windows)
            window->draw_window();

        auto engine = MagmaEngine{};
        vec2i32 mouse_pos{-1, -1};

        if (engine.window().get_input_interface_delta(MgmWindow::InputInterface::Key_P) == 1.0f)
            engine.notifications().push("Hello World");


        if ((engine.window().get_input_interface(MgmWindow::InputInterface::Key_CTRL) == 1.0f
            && engine.window().get_input_interface_delta(MgmWindow::InputInterface::Key_SPACE) == 1.0f)
            || (palette_open && engine.window().get_input_interface(MgmWindow::InputInterface::Key_ESC) == 1.0f)) {
            palette_open = !palette_open;
            mouse_pos = {
                static_cast<int>((engine.window().get_input_interface(MgmWindow::InputInterface::Mouse_POS_X) + 1.0f) * 0.5f * engine.window().get_size().x()),
                static_cast<int>((engine.window().get_input_interface(MgmWindow::InputInterface::Mouse_POS_Y) + 1.0f) * 0.5f * engine.window().get_size().y())
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
            system->in_editor_update(delta);
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

    void Editor::on_end_play() {
        for (const auto window : windows)
            delete window;
        Logging{"Editor"}.log("Editor closed");
    }
} // namespace mgm
