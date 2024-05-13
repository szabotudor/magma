#include "editor.hpp"
#include "engine.hpp"
#include "inspector.hpp"
#include "mgmwin.hpp"
#include "imgui.h"
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
        name = "Editor";
    }

    void Editor::on_begin_play() {
        ImGuiIO& io = ImGui::GetIO();
        static const ImWchar ranges[] = {
            0x0020, 0x00FF, // Basic Latin + Latin Supplement
            0x25A0, 0x25FF, // Geometric Shapes
            0,
        };
        io.Fonts->AddFontFromFileTTF("resources/fonts/Hack-Regular.ttf", 20.0f, nullptr, ranges);
        
        auto engine = MagmaEngine{};
        engine.systems().create<Inspector>();

        Logging{"Editor"}.log("Editor initialized");
    }

    void Editor::update(float delta) {
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);

        for (const auto window : windows)
            window->draw_window();

        auto engine = MagmaEngine{};
        vec2i32 mouse_pos{-1, -1};

        if (engine.window->get_input_interface(MgmWindow::InputInterface::Key_CTRL) == 1.0f
            && engine.window->get_input_interface_delta(MgmWindow::InputInterface::Key_SPACE) == 1.0f) {
            palette_open = !palette_open;
            mouse_pos = {
                static_cast<int>((engine.window->get_input_interface(MgmWindow::InputInterface::Mouse_POS_X) + 1.0f) * 0.5f * engine.window->get_size().x()),
                static_cast<int>((engine.window->get_input_interface(MgmWindow::InputInterface::Mouse_POS_Y) + 1.0f) * 0.5f * engine.window->get_size().y())
            };
            if (palette_open) {
                ImGui::SetNextWindowPos(ImVec2{static_cast<float>(mouse_pos.x() + 16), static_cast<float>(mouse_pos.y())});
                ImGui::SetNextWindowSize(ImVec2{-1.0f, 1.0f});
            }
        }

        if (!palette_open) {
            palette_window_height = std::lerp(palette_window_height, 0.0f, 0.2f);
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
        );

        ImGui::Text("Palette");
        ImGui::Separator();
        for (const auto& [type_id, system] : engine.systems().systems) {
            system->in_editor_update(delta);
            system->palette_options();
        }

        ImGui::PopStyleColor(3);

        if (palette_open) {
            const auto max_height = ImGui::GetCursorPosY();
            palette_window_height = std::lerp(palette_window_height, max_height, 0.2f);
        }

        ImGui::End();
        ImGui::PopFont();
    }

    void Editor::on_end_play() {
        for (const auto window : windows)
            delete window;
        Logging{"Editor"}.log("Editor closed");
    }

    template<>
    bool Inspector::inspect(const std::string&, EditorWindow*& value) {
        const auto checkbox_clicked = ImGui::Checkbox(value->name.c_str(), &value->open);
        if (checkbox_clicked && !value->open && value->remove_on_close)
            MagmaEngine{}.systems().get<Editor>().remove_window(value);

        return checkbox_clicked;
    }

    void Editor::palette_options() {
        auto& inspector = MagmaEngine{}.systems().get<Inspector>();
        inspector.inspect("Windows", windows);
    }
} // namespace mgm
