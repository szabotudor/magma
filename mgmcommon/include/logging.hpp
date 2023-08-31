#pragma once
#include <fstream>
#include <type_traits>

#include "file.hpp"


namespace mgm {
    class logging {
        bool is_init = false;
        std::ofstream file{};
        char* logger_name = nullptr;
        
        int log_entry(const char* str, const char* code = "\33[0m");
        void flush();
        
        public:

        explicit logging(const char* name);

        #if !defined(_MSC_VER)
        template<class ... Ts, typename std::enable_if<std::conjunction_v<std::is_convertible<Ts, const char*>...>, int>::type...>
        #else
        template<class ... Ts>
        #endif
        void log(Ts ... strs) {
            log_entry(logger_name, "\33[38;2;30;255;30m");
            log_entry("[LOG] ", "\33[38;2;10;100;255m");
            int xs[] = {log_entry(strs)...};
            flush();
        }
        #if !defined(_MSC_VER)
        template<class ... Ts, typename std::enable_if<std::conjunction_v<std::is_convertible<Ts, const char*>...>, int>::type...>
        #else
        template<class ... Ts>
        #endif
        void warning(Ts ... strs) {
            log_entry(logger_name, "\33[38;2;30;255;30m");
            log_entry("[WARNING] ", "\33[38;2;255;150;20m");
            int xs[] = {log_entry(strs)...};
            flush();
        }
        #if !defined(_MSC_VER)
        template<class ... Ts, typename std::enable_if<std::conjunction_v<std::is_convertible<Ts, const char*>...>, int>::type...>
        #else
        template<class ... Ts>
        #endif
        void error(Ts ... strs) {
            log_entry(logger_name, "\33[38;2;30;255;30m");
            log_entry("[ERROR] ", "\33[38;2;255;0;0m");
            int xs[] = {log_entry(strs, "\33[38;2;255;20;20m")...};
            flush();
        }

        ~logging();
    };
}
