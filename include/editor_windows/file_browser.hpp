#pragma once
#include "editor.hpp"
#include "engine.hpp"
#include "file.hpp"
#include <functional>


namespace mgm {
    class FileBrowser : public EditorWindow {
        std::vector<uint8_t> default_contents{};
        public:
        std::string file_name = "new_script";
        Path file_path = "project://";
        size_t selected_file = (size_t)-1;

        std::vector<Path> folders_here{};
        std::vector<Path> files_here{};

        std::function<void(const Path& path)> callback{};

        enum class Mode {
            // Open an existing file
            READ,
            // Save an existing file, or create a new one
            WRITE,
        } mode{};

        enum class Type {
            FILE,
            FOLDER,
        } type{};

        FileBrowser(Mode browser_mode, Type browser_type, const std::function<void(const Path& path)>& callback_function = {},
        const std::string& default_name = "New File", const std::vector<uint8_t>& default_file_contents = {})
        : default_contents{default_file_contents}, file_name{default_name}, callback{callback_function}, mode{browser_mode}, type{browser_type} {
            window_name = "File Browser";

            folders_here = MagmaEngine{}.file_io().list_folders(file_path);
            files_here = MagmaEngine{}.file_io().list_files(file_path);
        }

        void draw_contents() override;

        virtual ~FileBrowser() override {}
    };
}
