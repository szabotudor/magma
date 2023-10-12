#pragma once
#include <fstream>
#include <string>
#include <type_traits>

#include "file.hpp"


namespace mgm {
    class Logging {
        std::string logger_name{};
        
        int log_entry(const std::string&, const char* code = "\33[0m");
        void flush();
        
        public:

        explicit Logging(const std::string& name);

        template<class ... Ts, std::enable_if_t<std::conjunction_v<std::is_convertible<Ts, const std::string&>...>, bool> = true>
        void log(Ts ... strs) {
            log_entry(logger_name, "\33[38;2;30;255;30m");
            log_entry("[LOG] ", "\33[38;2;10;100;255m");
            int xs[] = {log_entry(strs)...};
            flush();
        }
        template<class ... Ts, std::enable_if_t<std::conjunction_v<std::is_convertible<Ts, const std::string&>...>, bool> = true>
        void warning(Ts ... strs) {
            log_entry(logger_name, "\33[38;2;30;255;30m");
            log_entry("[WARNING] ", "\33[38;2;255;150;20m");
            int xs[] = {log_entry(strs)...};
            flush();
        }
        template<class ... Ts, std::enable_if_t<std::conjunction_v<std::is_convertible<Ts, const std::string&>...>, bool> = true>
        void error(Ts ... strs) {
            log_entry(logger_name, "\33[38;2;30;255;30m");
            log_entry("[ERROR] ", "\33[38;2;255;0;0m");
            int xs[] = {log_entry(strs, "\33[38;2;255;20;20m")...};
            flush();
        }
    };
}
