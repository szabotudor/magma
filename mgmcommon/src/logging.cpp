#include "logging.hpp"
#include <cstring>
#include <iostream>
#include <fstream>
#include <filesystem>


namespace mgm {
    int logging::log_entry(const char* str, const char* code) {
        file << str;
        std::cout << code << str << "\33[0m";
        return 0;
    }

    void logging::flush() {
        file << std::endl;
        std::cout << std::endl;
    }

    logging::logging(const char* name) {
        if (!std::filesystem::exists(MgmFile::exe_dir() + "/log"))
            std::filesystem::create_directory(MgmFile::exe_dir() + "/log");
        file = std::ofstream(MgmFile::exe_dir() + "/log/" + name + ".log");
        
        size_t nlen = strlen(name) + 3;
        logger_name = new char[nlen];
        memcpy(logger_name + 1, name, nlen - 3);
        logger_name[0] = '[';
        logger_name[nlen - 2] = ']';
        logger_name[nlen - 1] = '\0';
    }

    logging::~logging() {
        file.close();
    }
}
