#pragma once
#include "editor.hpp"


namespace mgm {
    class ScriptEditor : public EditorWindow {
        Path path{};
        std::string content{};

        struct ColorData {
            int64_t start;
            int64_t end;
            vec3u8 color;
        };
        struct Line {
            int64_t start = 0;
            std::vector<ColorData> colors{};
        };
        std::vector<Line> lines{};

        Line& get_line(int64_t line_number) {
            if ((size_t)line_number >= lines.size()) {
                lines.resize((size_t)(line_number + 1));
            }
            return lines[(size_t)(line_number)];
        }
        const Line& get_line(int64_t line_number) const {
            if ((size_t)line_number >= lines.size()) {
                return lines[0];
            }
            return lines[(size_t)(line_number)];
        }

        int64_t line_count() const {
            return (int64_t)lines.size();
        }

        char& content_get(int64_t index) {
            if (index < 0) {
                return content[0];
            }
            if ((size_t)index >= content.size()) {
                return content[content.size() - 1];
            }
            return content[(size_t)index];
        }
        const char& content_get(int64_t index) const {
            if (index < 0) {
                return content[0];
            }
            if ((size_t)index >= content.size()) {
                return content[content.size() - 1];
            }
            return content[(size_t)index];
        }

        int64_t content_size() const {
            return (int64_t)content.size();
        }

        vec2i64 cursor_pos{};
        int64_t cursor = 0;
        int64_t old_cursor_x = 0;
        vec2i64 selection{};

        float max_inactivity_time = 3.0f;
        float time_since_last_edit = 0.0f;
        bool file_saved = true;

        void place_visual_cursor();
        void place_real_cursor();

        void detect_lines();

        void draw();
        void process_input();

    public:
        ScriptEditor(const Path& path, float save_after_inactivity_seconds = 3.0f);

        virtual void draw_contents() override;
        
        ~ScriptEditor() override;
    };
}
