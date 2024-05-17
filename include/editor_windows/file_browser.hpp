#pragma once
#include "editor.hpp"


namespace mgm {
    class FileBrowser : public EditorWindow {
        std::vector<uint8_t> default_contents{};
        public:
        std::string file_name = "new_script";
        Path file_path = Path::assets;
        size_t selected_file = (size_t)-1;

        enum class Mode {
            // Open an existing file
            READ,
            // Save an existing file, or create a new one
            WRITE,
        } mode{};

        FileBrowser(Mode mode, const std::string& default_name = "New File", const std::vector<uint8_t>& default_contents = {})
        : default_contents{default_contents}, file_name{default_name}, mode{mode} {
            name = "File Browser";
        }

        void draw_contents() override;

        virtual ~FileBrowser() override {}
    };
}
