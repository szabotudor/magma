#include "file.hpp"
#include "logging.hpp"
#include <fstream>
#include <ios>


namespace mgm {
    Path Path::engine_resources_dir{FileIO::exe_dir().direct_append("resources")};


    struct FileIO::Data {
        std::vector<std::ofstream> write_files{};
        std::vector<std::ifstream> read_files{};
    };


    FileIO::FileIO() {
        platform_data = new Data{};
    }

    std::string FileIO::read_text(const Path& path) {
        CHECK_PATH(path, "");

        const auto path_str = path.platform_path();
        auto file = std::ifstream{path_str};
        if (!file.is_open()) {
            Logging{"FileIO"}.error("Failed to open file: ", path_str);
            return "";
        }

        std::string result{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
        for (size_t i = 0; i < result.size(); i++) {
            if ((result[i] == '\r' && result[i + 1] == '\n')
                || (result[i] == '\n' && result[i + 1] == '\r')) {
                result.replace(i, 2, "\n");
            }
            else if (result[i] == '\r') {
                result[i] = '\n';
            }
        }
        return result;
    }
    void FileIO::write_text(const Path& path, const std::string& text) {
        CHECK_PATH(path, );

        const auto path_str = path.platform_path();
        auto file = std::ofstream{path_str};
        if (!file.is_open()) {
            Logging{"FileIO"}.error("Failed to open file: ", path_str);
            return;
        }

        file << text;
    }

    std::vector<uint8_t> FileIO::read_binary(const Path& path) {
        CHECK_PATH(path, {});

        const auto path_str = path.platform_path();
        auto file = std::ifstream{path_str, std::ios::binary};
        if (!file.is_open()) {
            Logging{"FileIO"}.error("Failed to open file: ", path_str);
            return {};
        }

        return std::vector<uint8_t>{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
    }
    void FileIO::write_binary(const Path& path, const std::vector<uint8_t>& data) {
        CHECK_PATH(path, );

        const auto path_str = path.platform_path();
        auto file = std::ofstream{path_str, std::ios::binary};
        if (!file.is_open()) {
            Logging{"FileIO"}.error("Failed to open file: ", path_str);
            return;
        }

        file.write(reinterpret_cast<const char*>(data.data()), (std::streamsize)data.size());
    }

    void FileIO::delete_file(const Path& path) {
        CHECK_PATH(path, );

        const auto path_str = path.platform_path();
        if (std::remove(path_str.c_str()) != 0)
            Logging{"FileIO"}.error("Failed to delete file: ", path_str);
    }


    void FileIO::begin_read_stream(const Path& path) {
        CHECK_PATH(path, );

        auto& read_file = platform_data->read_files.emplace_back();

        const auto path_str = path.platform_path();
        read_file = std::ifstream{path_str, std::ios::binary};
        if (!read_file.is_open()) {
            Logging{"FileIO"}.error("Failed to open file: ", path_str);
        }
    }
    void FileIO::read_stream(std::vector<uint8_t>& dst, size_t size) {
        if (platform_data->read_files.empty()) {
            Logging{"FileIO"}.error("No file open for reading. Call begin_read_stream first");
            return;
        }

        auto& read_file = platform_data->read_files.back();
        const auto start = dst.size();
        dst.resize(start + size);
        read_file.read(reinterpret_cast<char*>(dst.data() + start), (std::streamsize)size);
    }

    void FileIO::begin_write_stream(const Path& path) {
        CHECK_PATH(path, );

        auto& write_file = platform_data->write_files.emplace_back();

        const auto path_str = path.platform_path();
        write_file = std::ofstream{path_str, std::ios::binary};
        if (!write_file.is_open()) {
            Logging{"FileIO"}.error("Failed to open file: ", path_str);
        }
    }
    void FileIO::write_stream(const std::vector<uint8_t>& data) {
        if (this->platform_data->write_files.empty()) {
            Logging{"FileIO"}.error("No file open for writing. Call begin_write_stream first");
            return;
        }

        auto& write_file = this->platform_data->write_files.back();
        write_file.write(reinterpret_cast<const char*>(data.data()), (std::streamsize)data.size());
        write_file.flush();
    }

    void FileIO::end_read_stream() {
        if (platform_data->read_files.empty()) {
            Logging{"FileIO"}.error("No file to close, none are open for reading");
            return;
        }

        platform_data->read_files.pop_back();
    }
    void FileIO::end_write_stream() {
        if (platform_data->write_files.empty()) {
            Logging{"FileIO"}.error("No file to close, none are open for writing");
            return;
        }

        platform_data->write_files.pop_back();
    }

    FileIO::~FileIO() {
        if (!platform_data->read_files.empty())
            Logging{"FileIO"}.warning("FileIO destroyed with files still open for reading");
        if (!platform_data->write_files.empty())
            Logging{"FileIO"}.warning("FileIO destroyed with files still open for writing");

        delete platform_data;
    }
} // namespace mgm
