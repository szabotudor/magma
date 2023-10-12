#include "logging.hpp"
#include <iostream>


namespace mgm {
    int Logging::log_entry(const std::string& str, const char* code) {
        std::cout << code << str << "\33[0m";
        return 0;
    }

    void Logging::flush() {
        std::cout << std::endl;
    }
}
