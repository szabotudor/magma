#pragma once
#include <string>
#include <vector>


namespace mgm {
    struct Path {
        std::string data{};

        static Path exe_dir;
        static Path assets;
        static Path temp;

        Path(const std::string& path);
        Path(const char* path);

        Path() : data{assets.data} {}

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

        std::string file_name() const;
    };


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
