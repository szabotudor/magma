#include "editor_windows/script_editor.hpp"
#include "engine.hpp"
#include "imgui.h"
#include "logging.hpp"
#include "mgmwin.hpp"


namespace mgm {
    ScriptEditor::ScriptEditor(const Path& path, float save_after_inactivity_seconds) : path{path}, max_inactivity_time{save_after_inactivity_seconds} {
        name = path.file_name();

        MagmaEngine engine{};
        content = engine.file_io().read_text(path);
        detect_lines();
    }

    void ScriptEditor::detect_lines() {
        lines.clear();
        lines.emplace_back(Line{}); // First line
        for (size_t i = 0; i < content.size(); i++)
            if (content[i] == '\n' || i == content.size() - 1)
                lines.emplace_back(Line{ .start = i + 1 });
    }

    void ScriptEditor::draw_contents() {
        const auto start_pos = ImGui::GetCursorScreenPos();
        if (content.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(70, 70, 70, 255));
            ImGui::Text("Start typing to begin editing");
            ImGui::PopStyleColor();
        }
        if (lines.empty())
            detect_lines();

        if (cursor_pos.y() >= (int)lines.size())
            cursor_pos.y() = lines.size() - 1;
        if (cursor_pos.y() < (int)lines.size() - 1)
            if (cursor_pos.x() > (int)lines[cursor_pos.y() + 1].start)
                cursor_pos.x() = lines[cursor_pos.y() + 1].start;

        for (size_t l = 1; l < lines.size(); l++) {
            const auto& line = lines[l].start;
            const auto& last = lines[l - 1].start;
            const auto& colors = lines[l - 1].colors;
            if (line == last)
                continue;

            if (!colors.empty()) {
            }
            else
                ImGui::TextUnformatted(content.c_str() + last, content.c_str() + line);
        }
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            time_since_last_edit = 0.0f;
            auto mouse = ImGui::GetMousePos() - start_pos;
            cursor_pos.y() = std::min(static_cast<size_t>(mouse.y / ImGui::GetTextLineHeightWithSpacing()), lines.size() - 2);

            size_t i = 0;
            while (mouse.x > 0.0f) {
                const auto loc = lines[cursor_pos.y()].start + i;
                if (loc >= lines[cursor_pos.y() + 1].start || loc >= content.size())
                    break;
                char c = content[loc];
                mouse.x -= ImGui::CalcTextSize(&c, &c + 1).x;
                i++;
            }
            cursor_pos.x() = i;
            if (cursor_pos.x() > 0)
                if (content[lines[cursor_pos.y()].start + cursor_pos.x() - 1] == '\n')
                    cursor_pos.x()--;
        }

        if (static_cast<int>(time_since_last_edit * 0.5f) % 2 == 0) {
            ImVec2 pos_a = {
                ImGui::CalcTextSize(
                    content.c_str() + lines[cursor_pos.y()].start,
                    content.c_str() + lines[cursor_pos.y()].start + cursor_pos.x()
                ).x,
                ImGui::GetTextLineHeightWithSpacing() * cursor_pos.y()
            };
            ImVec2 pos_b = {
                pos_a.x,
                pos_a.y + ImGui::GetTextLineHeightWithSpacing()
            };

            ImGui::GetWindowDrawList()->AddRect(
                pos_a + start_pos,
                pos_b + start_pos,
                IM_COL32(255, 255, 255, 255)
            );
        }

        MagmaEngine engine{};

        time_since_last_edit += engine.delta_time();
        if (time_since_last_edit > max_inactivity_time && !file_saved) {
            file_saved = true;
            engine.file_io().write_text(path, content);
        }

        if (!engine.window().get_text_input().empty()) {
            time_since_last_edit = 0.0f;
            file_saved = false;
            content.insert(
                content.begin() + lines[cursor_pos.y()].start + cursor_pos.x(),
                engine.window().get_text_input().begin(),
                engine.window().get_text_input().end()
            );
            detect_lines();
            cursor_pos.x() += engine.window().get_text_input().size();
        }
        if (engine.window().get_input_interface_delta(MgmWindow::InputInterface::Key_ENTER) == 1.0f) {
            time_since_last_edit = 0.0f;
            file_saved = false;
            content.insert(content.begin() + lines[cursor_pos.y()].start + cursor_pos.x(), '\n');
            detect_lines();
            cursor_pos.x() = 0;
            cursor_pos.y()++;
        }

        if (time_since_last_edit > 1000.0f)
            time_since_last_edit = 0.0f;
    }

    ScriptEditor::~ScriptEditor() {
        if (!file_saved)
            MagmaEngine{}.file_io().write_text(path, content);
    }
}
