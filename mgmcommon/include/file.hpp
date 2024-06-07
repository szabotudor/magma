#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>


namespace mgm {
    struct Path {
        private:
        std::string parse_prefix() const {
            const auto it = data.find_first_of("://");
            if (it == std::string::npos) {
                return data;
            }
            if (it > 0) {
                const auto prefix = data.substr(0, it);
                const auto prefix_it = prefixes.find(prefix);
                if (prefix_it != prefixes.end())
                    return prefix_it->second.data + data.substr(it + 3);
            }
            if (it == 0)
                return prefixes.at("assets").data + data.substr(3);
            return data;
        }

        public:
        std::string data{};

        static Path exe_dir;
        static Path assets;
        static Path game_data;
        static Path temp;

        static const std::unordered_map<std::string, Path> prefixes;

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

        Path operator+(const Path &other) const;
        Path &operator+=(const Path &other);

        Path operator/(const Path &other) const {
            return *this + other;
        }
        Path &operator/=(const Path &other) {
            return *this += other;
        }

        Path operator-(const Path &other) const {
            const auto where = data.find(other.data);
            if (where == std::string::npos) {
                return *this;
            }
            if (where == 0) {
                return Path{data.substr(other.data.size())};
            }
            return Path{data.substr(0, where - 1)};
        }
        Path& operator-=(const Path &other) {
            return *this = *this - other;
        }

        Path back() const {
            const auto last_slash = data.find_last_of('/');
            if (last_slash == std::string::npos) {
                return Path{};
            }
            if (last_slash == 0) {
                return Path{"/"};
            }
            if (last_slash == data.size() - 1) {
                return Path{data.substr(0, data.find_last_of('/', last_slash - 1))};
            }
            return Path{data.substr(0, last_slash)};
        }

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

    inline const std::unordered_map<std::string, Path> Path::prefixes = {
        {"exe", exe_dir},
        {"assets", assets},
        {"game_data", game_data},
        {"temp", temp}
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
        static Path exe_dir();

        Data* platform_data = nullptr;

        public:

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
         * @brief Try to delete a file (unsuppoerted on some platforms, will log a warning if unsupported)
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
