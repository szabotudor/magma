#include "file.hpp"
#include <limits.h>
#include <unistd.h>


namespace mgm {
    Path::Path(const std::string& path) : data{path} {}
    Path::Path(const char* path) : data{path} {}

    std::string Path::platform_path() const {
        return parse_prefix();
    }

    Path Path::from_platform_path(const std::string &path) {
        return Path{path};
    }

    Path FileIO::exe_dir() {
        char buffer[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", buffer, sizeof(buffer));
        std::string executablePath(buffer, (count > 0) ? count : 0);
        while (executablePath.back() != '/')
            executablePath.pop_back();
        executablePath.pop_back();
        return executablePath;
    }
}
