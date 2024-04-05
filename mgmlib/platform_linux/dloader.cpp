#include "dloader.hpp"
#include "logging.hpp"
#include <cstring>

#if defined(__linux__)
#include <dlfcn.h>
#endif


namespace mgm {
    void* DLoader::void_sym(const char* sym_name) {
        return dlsym(lib, sym_name);
    }


    DLoader::DLoader(DLoader&& dl) {
        lib = dl.lib;
        dl.lib = nullptr;
    }

    DLoader& DLoader::operator=(DLoader&& dl) {
        lib = dl.lib;
        dl.lib = nullptr;

        return *this;
    }


    DLoader::DLoader(const char* file_path) {
        if (file_path != nullptr)
            load(file_path);
    }

    void DLoader::load(const char* file_path) {
        log.log("Trying to load dynamic library from \"", file_path, "\"");
        if (is_loaded()) unload();

        lib = dlopen(file_path, RTLD_LAZY | RTLD_GLOBAL);
        if (lib == nullptr) {
            log.error(dlerror());
            return;
        }

        log.log("Loaded dynamic library");
    }

    void DLoader::unload() {
        if (!is_loaded()) return;

        // TODO: This SEGFAULTS (sometimes)
        //dlclose(lib);
        lib = nullptr;

        log.log("Unloaded dynamic library");
    }

    DLoader::~DLoader() {
        unload();
    }
}
