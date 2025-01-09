#pragma once
#include "systems/editor.hpp"
#include "engine.hpp"
#include "file.hpp"
#include <functional>


namespace mgm {
    class FileBrowser : public EditorWindow {
        std::vector<uint8_t> default_contents{};
        public:
        std::string file_name = "new_script";
        std::string file_extension = "";
        Path file_path = "project://";
        size_t selected_file = (size_t)-1;
        std::function<void(const Path path)> callback{};
        bool allow_platform_paths = false;
        bool only_good_extensions = false;

        std::vector<Path> folders_here{};
        std::vector<Path> files_here{};


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

        struct Args {
            Mode mode = Mode::READ;
            Type type = Type::FILE;
            std::function<void(const Path path)> callback{};
            bool allow_paths_outside_project = false;
            std::string default_file_name = "New File";
            std::string default_file_extension = "";
            std::vector<uint8_t> default_file_contents{};
            bool only_show_files_with_proper_extension = false;
        };

        FileBrowser(const Args& args)
        : default_contents{args.default_file_contents}, file_name{args.default_file_name}, file_extension{args.default_file_extension}, callback{args.callback},
        allow_platform_paths{args.allow_paths_outside_project}, only_good_extensions{args.only_show_files_with_proper_extension}, mode{args.mode}, type{args.type} {
            window_name = "File Browser";

            folders_here = MagmaEngine{}.file_io().list_folders(file_path);
            files_here = MagmaEngine{}.file_io().list_files(file_path);
        }

        void draw_contents() override;

        virtual ~FileBrowser() override {}
    };
}
