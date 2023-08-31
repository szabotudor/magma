#include "file.hpp"

#if defined(__linux__)
#include <limits.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <Windows.h>
#endif


namespace mgm {
    std::string MgmFile::exe_dir() {
        #if defined(__linux__)
        char buffer[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", buffer, sizeof(buffer));
        std::string executablePath(buffer, (count > 0) ? count : 0);
        while (executablePath.back() != '/')
            executablePath.pop_back();
        executablePath.pop_back();
        return executablePath;
        #elif defined(WIN32)
        return std::string();
        #endif
    }
}
