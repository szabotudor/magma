#include "file.hpp"

#include <limits.h>
#include <unistd.h>


namespace mgm {
    std::string Path::platform_path() const {
        return data;
    }

    std::string FileIO::exe_dir() {
        char buffer[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", buffer, sizeof(buffer));
        std::string executablePath(buffer, (count > 0) ? count : 0);
        while (executablePath.back() != '/')
            executablePath.pop_back();
        executablePath.pop_back();
        return executablePath;
    }
}
