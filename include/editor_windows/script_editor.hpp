#pragma once
#include "editor.hpp"


namespace mgm {
    class ScriptEditor : public EditorWindow {
        Path path{};
        std::string content{};

        struct ColorData {
            size_t start;
            size_t end;
            vec3u8 color;
        };
        struct Line {
            size_t start = 0;
            std::vector<ColorData> colors{};
        };
        std::vector<Line> lines{};

        vec2i32 cursor_pos{};

        float max_inactivity_time = 3.0f;
        float time_since_last_edit = 0.0f;
        bool file_saved = true;

        void detect_lines();

        void display_line(const std::string& line, const std::vector<ColorData>& colors);

    public:
        ScriptEditor(const Path& path, float save_after_inactivity_seconds = 3.0f);

        virtual void draw_contents() override;
        
        ~ScriptEditor() override;
    };
}
