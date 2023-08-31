#include "dloader.hpp"
#include "logging.hpp"
#include <cstring>

#if defined(__linux__)
#include <dlfcn.h>
#endif


namespace mgm {
    #if defined(__linux__)
    void* DLoader::void_sym(const char* sym_name) {
        return dlsym(lib, sym_name);
    }


    DLoader::DLoader(const DLoader& dl) {
        pathlen = dl.pathlen;
        path = new char[pathlen];
        memcpy(path, dl.path, pathlen);
        lib = dlopen(path, RTLD_LAZY);
        if (lib == nullptr) {
            log.error(dlerror());
            return;
        }
    }

    DLoader::DLoader(DLoader&& dl) {
        lib = dl.lib;
        dl.lib = nullptr;

        path = dl.path;
        dl.path = nullptr;
    }

    DLoader& DLoader::operator=(const DLoader& dl) {
        pathlen = dl.pathlen;
        path = new char[pathlen];
        memcpy(path, dl.path, pathlen);
        lib = dlopen(path, RTLD_LAZY);
        if (lib == nullptr) {
            log.error(dlerror());
            return *this;
        }
        return *this;
    }

    DLoader& DLoader::operator=(DLoader&& dl) {
        lib = dl.lib;
        dl.lib = nullptr;

        path = dl.path;
        dl.path = nullptr;

        return *this;
    }


    DLoader::DLoader(const char* file_path) {
        if (file_path != nullptr)
            load(file_path);
    }

    void DLoader::load(const char* file_path) {
        log.log("Trying to load dynamic library from \"", file_path, "\"");
        if (is_loaded()) unload();

        lib = dlopen(file_path, RTLD_LAZY);
        if (lib == nullptr) {
            log.error(dlerror());
            return;
        }

        pathlen = strlen(file_path) + 1;
        path = new char[pathlen];
        memcpy(path, file_path, pathlen);

        log.log("Loaded dynamic library");
    }

    void DLoader::unload() {
        log.log("Trying to unload dynamic library from \"", path, "\"");
        if (!is_loaded()) return;

        dlclose(lib);
        lib = nullptr;

        delete[] path;
        path = nullptr;

        log.log("Unloaded dynamic library");
    }

    bool DLoader::is_loaded() {
        return lib != nullptr;
    }

    DLoader::~DLoader() {
        unload();
    }
    #endif
}
