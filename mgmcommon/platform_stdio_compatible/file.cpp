#include <fstream>
#include "file.hpp"
#include "logging.hpp"


namespace mgm {
    Path Path::exe_dir{FileIO::exe_dir()};
    Path Path::assets{Path::exe_dir + Path{"assets/"}};
    Path Path::temp{Path::exe_dir + Path{"temp/"}};


    Path &Path::operator+=(const Path &other) {
      if (data.back() == '/' && other.data.front() == '/') {
        data += other.data.substr(1);
      } else if (data.back() != '/' && other.data.front() != '/') {
        data += "/" + other.data;
      } else {
        data += other.data;
      }
      return *this;
    }
    Path Path::operator+(const Path &other) const {
      if (data.back() == '/' && other.data.front() == '/') {
        return Path{data + other.data.substr(1)};
      } else if (data.back() != '/' && other.data.front() != '/') {
        return Path{data + "/" + other.data};
      }
      return Path{data + other.data};
    }


    struct FileIO::Data {
        std::vector<std::ofstream> write_files{};
        std::vector<std::ifstream> read_files{};
    };


    FileIO::FileIO() {
        platform_data = new Data{};
    }

    std::string FileIO::read_text(const Path &path) {
        const auto path_str = path.platform_path();
        auto file = std::ifstream{path_str};
        if (!file.is_open()) {
            Logging{"FileIO"}.error("Failed to open file: ", path_str);
            return "";
        }

        return std::string{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
    }
    void FileIO::write_text(const Path &path, const std::string &text) {
        const auto path_str = path.platform_path();
        auto file = std::ofstream{path_str};
        if (!file.is_open()) {
            Logging{"FileIO"}.error("Failed to open file: ", path_str);
            return;
        }

        file << text;
    }

    std::vector<uint8_t> FileIO::read_binary(const Path &path) {
        const auto path_str = path.platform_path();
        auto file = std::ifstream{path_str, std::ios::binary};
        if (!file.is_open()) {
            Logging{"FileIO"}.error("Failed to open file: ", path_str);
            return {};
        }

        return std::vector<uint8_t>{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
    }
    void FileIO::write_binary(const Path &path, const std::vector<uint8_t> &data) {
        const auto path_str = path.platform_path();
        auto file = std::ofstream{path_str, std::ios::binary};
        if (!file.is_open()) {
            Logging{"FileIO"}.error("Failed to open file: ", path_str);
            return;
        }

        file.write(reinterpret_cast<const char*>(data.data()), data.size());
    }

    bool FileIO::exists(const Path &path) {
        return std::ifstream{path.platform_path()}.is_open();
    }

    void FileIO::delete_file(const Path &path) {
        const auto path_str = path.platform_path();
        if (std::remove(path_str.c_str()) != 0)
            Logging{"FileIO"}.error("Failed to delete file: ", path_str);
    }


    void FileIO::begin_read_stream(const Path &path) {
        auto& read_file = platform_data->read_files.emplace_back();

        const auto path_str = path.platform_path();
        read_file = std::ifstream{path_str, std::ios::binary};
        if (!read_file.is_open()) {
            Logging{"FileIO"}.error("Failed to open file: ", path_str);
        }
    }
    void FileIO::read_stream(std::vector<uint8_t> &dst, size_t size) {
        if (platform_data->read_files.empty()) {
            Logging{"FileIO"}.error("No file open for reading. Call begin_read_stream first");
            return;
        }

        auto& read_file = platform_data->read_files.back();
        const auto start = dst.size();
        dst.resize(start + size);
        read_file.read(reinterpret_cast<char*>(dst.data() + start), size);
    }

    void FileIO::begin_write_stream(const Path &path) {
        auto& write_file = platform_data->write_files.emplace_back();

        const auto path_str = path.platform_path();
        write_file = std::ofstream{path_str, std::ios::binary};
        if (!write_file.is_open()) {
            Logging{"FileIO"}.error("Failed to open file: ", path_str);
        }
    }
    void FileIO::write_stream(const std::vector<uint8_t> &data) {
        if (this->platform_data->write_files.empty()) {
            Logging{"FileIO"}.error("No file open for writing. Call begin_write_stream first");
            return;
        }

        auto& write_file = this->platform_data->write_files.back();
        write_file.write(reinterpret_cast<const char*>(data.data()), data.size());
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
