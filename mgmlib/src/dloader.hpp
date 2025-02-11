#pragma once
#include "logging.hpp"


namespace mgm {
    class DLoader {
        void* lib = nullptr;

        Logging log{"dloader"};

        void* void_sym(const char* sym_name);

        public:
        DLoader(const DLoader&) = delete;
        DLoader(DLoader&&);
        DLoader& operator=(const DLoader&) = delete;
        DLoader& operator=(DLoader&&);

        DLoader(const char* file_path = nullptr);

        /**
         * @brief Load a shared/dynamic library
         * 
         * @param file_path Path to the so/dll file
         */
        void load(const char* file_path);

        /**
         * @brief Read a symbol from the library if a library is loaded
         * 
         * @tparam T Type of the symbol to load
         * @param sym_name String with the symbol name
         * @param sym Pointer to a symbol adress to fill
         */
        template<class T>
        requires std::is_pointer_v<T>
        void sym(const char* sym_name, T* sym) {
            if (!is_loaded()) return;
            *sym = (T)void_sym(sym_name);
        }

        /**
         * @brief Unload the library
         */
        // !!! THIS SEGFAULTS !!!
        void unload();

        /**
         * @brief Check if any library is loaded
         */
        bool is_loaded() const { return lib != nullptr; }

        ~DLoader();
    };
}
