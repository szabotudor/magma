#include "editor_windows/script_editor.hpp"
#include "engine.hpp"
#include "imgui.h"
#include "logging.hpp"
#include "mgmwin.hpp"
#include "notifications.hpp"


namespace mgm {
    ScriptEditor::ScriptEditor(const Path& path, float save_after_inactivity_seconds) : path{path}, max_inactivity_time{save_after_inactivity_seconds} {
        window_name = path.file_name();

        MagmaEngine engine{};
        content = engine.file_io().read_text(path);
        detect_lines();
    }

    void ScriptEditor::place_visual_cursor() {
        if (cursor > static_cast<int64_t>(content.size()))
            cursor = static_cast<int64_t>(content.size());

        int64_t start = 0;
        int64_t end = static_cast<int64_t>(lines.size()) - 1;
        while (start < end) {
            const int64_t mid = (start + end) / 2;
            if (lines[mid].start > cursor)
                end = mid;
            else
                start = mid + 1;
        }
        cursor_pos.y() = start - 1;
        cursor_pos.x() = cursor - lines[start - 1].start;
    }

    void ScriptEditor::place_real_cursor() {
        cursor = lines[cursor_pos.y()].start + cursor_pos.x();
    }

    void ScriptEditor::detect_lines() {
        lines.clear();
        lines.emplace_back(Line{}); // First line
        const auto content_size = static_cast<int64_t>(content.size());
        for (int64_t i = 0; i < content_size; i++)
            if (content[i] == '\n')
                lines.emplace_back(Line{ .start = i + 1 });
        lines.emplace_back(Line{ .start = content_size });
    }

    void ScriptEditor::draw() {
        const auto start_pos = ImGui::GetCursorScreenPos();
        if (content.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(70, 70, 70, 255));
            ImGui::Text("\tStart typing to begin editing");
            ImGui::PopStyleColor();
            ImGui::SetCursorPos(start_pos);
            return;
        }

        const auto scroll = ImGui::GetScrollY();
        const auto line_height = ImGui::GetTextLineHeightWithSpacing();
        const auto line_height_no_spacing = ImGui::GetTextLineHeight();
        const auto winodw_height = ImGui::GetWindowHeight();
        const auto line_start = static_cast<int64_t>(scroll / line_height);
        ImGui::Dummy({ 0.0f, line_start * line_height });

        const int64_t line_end = std::min(static_cast<int64_t>(lines.size()), line_start + static_cast<int64_t>(winodw_height / line_height) + 2) - 1;

        const auto max_line_num_width = ImGui::CalcTextSize((std::to_string(line_end) + "  ").c_str()).x;

        for (int64_t l = 1 + line_start; l < static_cast<int64_t>(lines.size()); l++) {
            const auto& line = lines[l].start;
            const auto& last = lines[l - 1].start;
            const auto& colors = lines[l - 1].colors;

            if (!colors.empty()) {
            }
            else {
                ImGui::TextUnformatted(std::to_string(l).c_str());
                ImGui::SameLine();
                ImGui::Dummy({ max_line_num_width - ImGui::GetCursorPosX(), line_height_no_spacing });
                ImGui::SameLine();
                ImGui::TextUnformatted(content.c_str() + last, content.c_str() + line);
            }
            if ((l - line_start) * line_height > winodw_height - line_height * 2.0f) {
                ImGui::Dummy({ 0.0f, (static_cast<int64_t>(lines.size()) - l - 1) * line_height - (line_height - line_height_no_spacing) * (l == static_cast<int64_t>(lines.size() - 1) ? 3.0f : 1.0f) });
                break;
            }
        }
        ImGui::Dummy({ 0.0f, winodw_height - line_height_no_spacing * 3.0f });

        // Process input code in draw cause it's easier, and it's not really input processing, just setting the cursor position
        const bool window_hovered = ImGui::IsWindowHovered()
            && ImGui::IsMouseHoveringRect(
                start_pos + ImVec2{0.0f, scroll},
                start_pos + ImVec2{ImGui::GetWindowWidth(), ImGui::GetWindowHeight() + scroll}
            );
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && window_hovered) {
            time_since_last_edit = 0.0f;
            auto mouse = ImGui::GetMousePos() - start_pos - ImVec2{max_line_num_width, 0.0f};
            cursor_pos.y() = std::min(static_cast<size_t>(mouse.y / line_height), lines.size() - 2);

            int64_t i = 0;
            while (mouse.x > 0.0f) {
                const auto loc = lines[cursor_pos.y()].start + i;
                if (loc >= static_cast<int64_t>(content.size()))
                    break;
                if (content[loc] == '\n')
                    break;
                mouse.x -= ImGui::CalcTextSize(&content[loc], &content[loc] + 1).x;
                i++;
            }
            cursor_pos.x() = i;
            if (cursor_pos.x() > 0)
                if (content[lines[cursor_pos.y()].start + cursor_pos.x() - 1] == '\n')
                    cursor_pos.x()--;
            
            old_cursor_x = cursor_pos.x();
            place_real_cursor();
        }

        if (static_cast<int>(time_since_last_edit * 3.0f) % 2 == 0) {
            ImVec2 pos_a = {
                ImGui::CalcTextSize(
                    content.c_str() + lines[cursor_pos.y()].start,
                    content.c_str() + lines[cursor_pos.y()].start + cursor_pos.x()
                ).x,
                line_height * cursor_pos.y()
            };
            ImVec2 pos_b = {
                pos_a.x,
                pos_a.y + line_height
            };

            ImGui::GetWindowDrawList()->AddRect(
                pos_a + start_pos + ImVec2{max_line_num_width, 0.0f},
                pos_b + start_pos + ImVec2{max_line_num_width, 0.0f},
                IM_COL32(255, 255, 255, 255)
            );
        }

        const auto line_num_separator_col = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg) + ImVec4{0.1f, 0.1f, 0.1f, 0.0f};
        const auto line_num_separator_col32 = IM_COL32(line_num_separator_col.x * 255, line_num_separator_col.y * 255, line_num_separator_col.z * 255, 255);
        const auto line_num_separator_width = ImGui::CalcTextSize(" ").x;
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2{max_line_num_width - line_num_separator_width * 1.5f, scroll - line_height} + start_pos,
            ImVec2{max_line_num_width - line_num_separator_width * 0.75f, winodw_height + scroll} + start_pos,
            line_num_separator_col32
        );
    }

    void ScriptEditor::process_input() {
        MagmaEngine engine{};

        time_since_last_edit += engine.delta_time();
        if (time_since_last_edit > max_inactivity_time && !file_saved) {
            file_saved = true;
            engine.file_io().write_text(path, content);
            engine.notifications().push("File \"" + path.data + "\" saved");
        }

        if (!engine.window().get_text_input().empty()) {
            time_since_last_edit = 0.0f;
            file_saved = false;
            content.insert(
                content.begin() + cursor,
                engine.window().get_text_input().begin(),
                engine.window().get_text_input().end()
            );
            cursor += engine.window().get_text_input().size();
            detect_lines();
            place_visual_cursor();
        }

        for (const auto& event : engine.window().get_input_events()) {
            if (event.mode == MgmWindow::InputEvent::Mode::PRESS) {
                if (event.input == MgmWindow::InputInterface::Key_ENTER) {
                    time_since_last_edit = 0.0f;
                    file_saved = false;
                    content.insert(content.begin() + cursor, '\n');
                    cursor++;
                    detect_lines();
                    place_visual_cursor();
                }
                if (event.input == MgmWindow::InputInterface::Key_BACKSPACE) {
                    time_since_last_edit = 0.0f;
                    file_saved = false;
                    if (cursor == 0)
                        continue;
                    cursor--;
                    content.erase(content.begin() + cursor);
                    detect_lines();
                    place_visual_cursor();
                }
                if (event.input == MgmWindow::InputInterface::Key_DELETE) {
                    time_since_last_edit = 0.0f;
                    file_saved = false;
                    if (cursor >= static_cast<int64_t>(content.size()) - 1)
                        continue;
                    content.erase(content.begin() + cursor);
                    detect_lines();
                    place_visual_cursor();
                }

                if (event.input == MgmWindow::InputInterface::Key_ARROW_UP) {
                    time_since_last_edit = 0.0f;
                    if (cursor_pos.y() > 0)
                        cursor_pos.y()--;
                    if (cursor_pos.x() < old_cursor_x)
                        cursor_pos.x() = old_cursor_x;
                    if (cursor_pos.x() > lines[cursor_pos.y() + 1].start - lines[cursor_pos.y()].start - 1)
                        cursor_pos.x() = lines[cursor_pos.y() + 1].start - lines[cursor_pos.y()].start - 1;
                    place_real_cursor();
                }
                if (event.input == MgmWindow::InputInterface::Key_ARROW_DOWN) {
                    time_since_last_edit = 0.0f;
                    if (cursor_pos.y() < static_cast<int64_t>(lines.size()) - 2)
                        cursor_pos.y()++;
                    if (cursor_pos.x() < old_cursor_x)
                        cursor_pos.x() = old_cursor_x;
                    if (cursor_pos.x() > lines[cursor_pos.y() + 1].start - lines[cursor_pos.y()].start - 1)
                        cursor_pos.x() = lines[cursor_pos.y() + 1].start - lines[cursor_pos.y()].start - 1;
                    if (cursor_pos.x() < 0)
                        cursor_pos.x() = 0; // Bandaid fix cause I can't be bothered to figure out why the last line is so broken when it's empty
                    place_real_cursor();
                }
                if (event.input == MgmWindow::InputInterface::Key_ARROW_LEFT) {
                    time_since_last_edit = 0.0f;
                    if (cursor > 0)
                        cursor--;
                    place_visual_cursor();
                    old_cursor_x = cursor_pos.x();
                }
                if (event.input == MgmWindow::InputInterface::Key_ARROW_RIGHT) {
                    time_since_last_edit = 0.0f;
                    if (cursor < static_cast<int64_t>(content.size()))
                        cursor++;
                    place_visual_cursor();
                    old_cursor_x = cursor_pos.x();
                }
            }
        }

        if (time_since_last_edit > 1000.0f)
            time_since_last_edit = 0.0f;
    }

    void ScriptEditor::draw_contents() {
        if (lines.empty())
            detect_lines();

        draw();
        process_input();
    }

    ScriptEditor::~ScriptEditor() {
        if (!file_saved)
            MagmaEngine{}.file_io().write_text(path, content);
    }
}
