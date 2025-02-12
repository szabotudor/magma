#include "dloader.hpp"

#include "../../mgmwin/platform_windows/native_window.hpp"


namespace mgm {
    void* DLoader::void_sym(const char* sym_name) {
        auto res = reinterpret_cast<void*>(GetProcAddress(static_cast<HINSTANCE>(lib), sym_name));
        if (res == nullptr) {
            log.log(std::to_string(GetLastError()));
        }
        return res;
    }

    DLoader::DLoader(const char* file_path) {
        if (file_path != nullptr)
            load(file_path);
    }

    void DLoader::load(const char* file_path) {
        lib = LoadLibrary(file_path);
    }

    void DLoader::unload() {
        lib = nullptr;
    }

    DLoader::~DLoader() = default;
} // namespace mgm
