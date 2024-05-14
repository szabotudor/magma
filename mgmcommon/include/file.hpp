#pragma once
#include <string>
#include <vector>


namespace mgm {
    struct Path {
        std::string data{};

        static Path exe_dir;
        static Path assets;
        static Path temp;

        constexpr Path(const std::string& path) {
            data = path;
        }
        constexpr Path(const char* path) {
            data = path;
        }

        Path operator+(const Path &other) const;
        Path &operator+=(const Path &other);

        std::string platform_path() const;
    };


    class FileIO {
        friend struct Path;

        struct Data;
        static std::string exe_dir();

        Data* platform_data = nullptr;

        public:

        FileIO();

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
