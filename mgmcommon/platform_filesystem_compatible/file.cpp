#include "file.hpp"
#include "logging.hpp"
#include <filesystem>


namespace mgm {
    std::vector<Path> FileIO::list_files(const Path& path, bool recursive) {
        CHECK_PATH(path, {});

        std::vector<Path> files{};

        const auto path_str = path.platform_path();
        if (!std::filesystem::exists(path_str))
            return files;

        auto dir = std::filesystem::directory_iterator{path_str};
        for (const auto& entry : dir) {
            if (entry.is_regular_file())
                files.emplace_back(entry.path().string()).make_platform_independent();
            else if (recursive && entry.is_directory()) {
                auto sub_files = list_files(entry.path().string(), true);
                files.insert(files.end(), sub_files.begin(), sub_files.end());
            }
        }

        return files;
    }

    std::vector<Path> FileIO::list_folders(const Path& path, bool recursive) {
        CHECK_PATH(path, {});

        std::vector<Path> folders{};

        const auto path_str = path.platform_path();
        if (!std::filesystem::exists(path_str))
            return folders;

        auto dir = std::filesystem::directory_iterator{path_str};
        for (const auto& entry : dir) {
            if (entry.is_directory()) {
                folders.emplace_back(entry.path().string()).make_platform_independent();
                if (recursive) {
                    auto sub_folders = list_folders(entry.path().string(), true);
                    folders.insert(folders.end(), sub_folders.begin(), sub_folders.end());
                }
            }
        }

        return folders;
    }

    void FileIO::create_folder(const Path& path) {
        CHECK_PATH(path, );

        const auto path_str = path.platform_path();
        if (!std::filesystem::exists(path.back().platform_path())) {
            Logging{"FileIO"}.error("Folder \"", path_str, "\" doesn't exist: ", path.back().platform_path(), "\n\tCannot create new folder: ", path_str);
            return;
        }
        if (!std::filesystem::create_directory(path_str))
            Logging{"FileIO"}.error("Failed to create folder: ", path_str);
    }

    bool FileIO::exists(const Path& path) {
        CHECK_PATH(path, false);
        return std::filesystem::exists(path.platform_path());
    }
} // namespace mgm
