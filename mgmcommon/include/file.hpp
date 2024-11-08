#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>


namespace mgm {
    class FileIO;

    struct Path {
        friend class FileIO;

        private:
        std::string parse_prefix() const;
        Path as_platform_independent() const;
        void make_platform_independent() {
            data = as_platform_independent().data;
        }

        public:
        std::string data{};

        static Path project_dir;
        static Path assets_dir;
        static Path game_data_dir;
        static Path engine_resources_dir;

        static void setup_project_dirs(const std::string& platform_project_dir, const std::string& platform_assets_dir, const std::string& platform_game_data_dir);

        static const std::unordered_map<std::string, const Path*> prefixes;

        Path(const std::string& path);
        Path(const char* path);

        Path() {}

        Path(const Path& other) : data{other.data} {}
        Path(Path&& other) : data{std::move(other.data)} {}
        Path& operator=(const Path& other) {
            if (this == &other)
                return *this;
            data = other.data;
            return *this;
        }
        Path& operator=(Path&& other) {
            if (this == &other)
                return *this;
            data = std::move(other.data);
            return *this;
        }

        Path direct_append(const Path& other) const;
        Path direct_remove(const Path& other) const;

        Path operator+(const Path &other) const {
            return direct_append(other).as_platform_independent();
        }
        Path &operator+=(const Path &other) {
            *this = this->direct_append(other).as_platform_independent();
            return *this;
        }

        Path operator/(const Path &other) const {
            return *this + other;
        }
        Path &operator/=(const Path &other) {
            return *this += other;
        }

        Path operator-(const Path &other) const {
            return direct_remove(other).as_platform_independent();
        }
        Path& operator-=(const Path &other) {
            return *this = *this - other;
        }

        bool operator==(const Path& other) const {
            return as_platform_independent().data == other.as_platform_independent().data;
        }
        bool operator!=(const Path& other) const {
            return !(*this == other);
        }

        Path back() const;

        std::string platform_path() const;
        
        static Path from_platform_path(const std::string& path);

        std::string file_name() const;

        bool empty() const {
            return data.empty();
        }

        enum class Validity {
            // The path is empty (not valid, but not invalid either)
            EMPTY,

            // The path is invalid
            INVALID,

            // The path is valid, but it points to a file or folder outside of the allowed directories.
            // This is fine most of the time, but could be a problem on some platforms
            OUTSIDE_ALLOWED,

            // The path is valid and points to a file or folder inside the allowed directories
            VALID
        };
        Validity validity() const {
            if (data.empty())
                return Validity::EMPTY;
            if (data.find_first_of("://") == std::string::npos)
                return Validity::OUTSIDE_ALLOWED;
            return Validity::VALID;
        }
    };

    inline const std::unordered_map<std::string, const Path*> Path::prefixes = {
        {"project", &Path::project_dir},
        {"assets", &Path::assets_dir},
        {"data", &Path::game_data_dir},
        {"resources", &Path::engine_resources_dir}
    };

#define CHECK_PATH(path, default_return_value) \
    if (path.validity() == Path::Validity::OUTSIDE_ALLOWED) \
        Logging{"FileIO"}.warning("Path is outside allowed directories: ", path.data, "\n\tThis is fine on most platforms, but could be a problem on some"); \
    if (path.validity() == Path::Validity::INVALID) { \
        Logging{"FileIO"}.error("Invalid path: ", path.data); \
        return default_return_value; \
    }


    class FileIO {
        friend struct Path;

        struct Data;

        Data* platform_data = nullptr;

        public:
        static Path exe_dir();

        FileIO();

        /**
         * @brief List all files in a directory
         * 
         * @param path The path to the directory
         * @param recursive Whether to list files in subdirectories
         * @return std::vector<Path> The list of files
         */
        std::vector<Path> list_files(const Path& path, bool recursive = false);

        /**
         * @brief List all folders in a directory
         * 
         * @param path The path to the directory
         * @param recursive Whether to list folders in subdirectories
         * @return std::vector<Path> The list of folders
         */
        std::vector<Path> list_folders(const Path& path, bool recursive = false);

        /**
         * @brief Create a folder at the specified path
         * 
         * @param path The path to the folder
         */
        void create_folder(const Path& path);

        /**
         * @brief Reads the text from a file
         * 
         * @param path The path to the file
         * @return std::string The text contents of the file
         */
        std::string read_text(const Path& path);

        /**
         * @brief Writes text to a file
         * 
         * @param path The path to the file
         * @param text The text to write
         */
        void write_text(const Path& path, const std::string& text);

        /**
         * @brief Reads binary data from a file
         * 
         * @param path The path to the file
         * @return std::vector<uint8_t> The binary data
         */
        std::vector<uint8_t> read_binary(const Path& path);

        /**
         * @brief Writes binary data to a file
         * 
         * @param path The path to the file
         * @param data The binary data to write
         */
        void write_binary(const Path& path, const std::vector<uint8_t>& data);

        /**
         * @brief Checks if a file exists
         * 
         * @param path The path to the file
         * @return true If the file exists
         * @return false If the file does not exist
         */
        bool exists(const Path& path);

        /**
         * @brief Try to delete a file (unsupported on some platforms, will log a warning if unsupported)
         * 
         * @param path The path to the file
         */
        void delete_file(const Path& path);


        /**
         * @brief Open a file for reading in binary mode, as a stream. Useful for extremely large files that don't fit in memory
         * 
         * @param path The path to the file
         */
        void begin_read_stream(const Path& path);

        /**
         * @brief Read a chunk of data from the stream
         * 
         * @param data The buffer to read the data into
         * @param size The size of the buffer
         */
        void read_stream(std::vector<uint8_t>& dst, size_t size);

        /**
         * @brief Open a file for writing in binary mode, as a stream. Useful for extremely large files, or for files that are being written to over time
         * 
         * @param path The path to the file
         */
        void begin_write_stream(const Path& path);

        /**
         * @brief Write a chunk of data to the stream
         * 
         * @param data The data to write
         */
        void write_stream(const std::vector<uint8_t>& src);

        /**
         * @brief Close the last file opened for reading
         */
        void end_read_stream();

        /**
         * @brief Close the last file opened for writing
         */
        void end_write_stream();

        ~FileIO();
    };
}
