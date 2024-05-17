#include "editor_windows/script_editor.hpp"
#include "engine.hpp"
#include "imgui.h"
#include "logging.hpp"
#include "mgmwin.hpp"


namespace mgm {
    ScriptEditor::ScriptEditor(const Path& path) : path{path} {
        name = path.file_name();
        
        MagmaEngine engine{};
        content = engine.file_io().read_text(path);
        detect_lines();
    }

    void ScriptEditor::detect_lines() {
        lines.clear();
        for (size_t i = 0; i < content.size(); i++)
            if (content[i] == '\n')
                lines.emplace_back(Line{ .start = i + 1 });
    }

    void ScriptEditor::draw_contents() {
        size_t last = 0;
        for (const auto& [start, colors] : lines) {
            if (start == last)
                continue;

            if (!colors.empty()) {
            }
            else {
                ImGui::TextUnformatted(content.c_str() + last, content.c_str() + start);
            }

            last = start;
        }

        ImVec2 pos_a = {
            ImGui::CalcTextSize(
                content.c_str() + lines[cursor_pos.x()].start,
                content.c_str() + lines[cursor_pos.x()].start - 1
            ).x,
            ImGui::GetTextLineHeightWithSpacing() * cursor_pos.y()
        };
        ImVec2 pos_b = {
            pos_a.x,
            pos_a.y + ImGui::GetTextLineHeightWithSpacing()
        };

        ImGui::GetWindowDrawList()->AddRect(
            ImGui::GetCursorScreenPos() + pos_a,
            ImGui::GetCursorScreenPos() + pos_b,
            IM_COL32(255, 255, 255, 255)
        );

        MagmaEngine engine{};

        time_since_last_edit += engine.delta_time();
        if (time_since_last_edit > 3.0f) {
            engine.file_io().write_text(path, content);
            Logging{"ScriptEditor"}.log("Saved file: ", path.platform_path());
        }

        if (!engine.window().get_text_input().empty()) {
            time_since_last_edit = 0.0f;
            content.insert(
                content.begin() + lines[cursor_pos.x()].start + cursor_pos.y(),
                engine.window().get_text_input().begin(),
                engine.window().get_text_input().end()
            );
        }
    }

    ScriptEditor::~ScriptEditor() {
    }
}
