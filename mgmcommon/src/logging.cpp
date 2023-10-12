#include "logging.hpp"
#include <cstring>
#include <iostream>


namespace mgm {
    Logging::Logging(const std::string& name) {
        logger_name.reserve(name.size() + 2);
        logger_name.resize(0);
        logger_name.push_back('[');
        logger_name.insert(logger_name.end(), name.begin(), name.end());
        logger_name.push_back(']');
    }
}
