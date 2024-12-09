#include "file.hpp"
#include <Windows.h>

namespace mgm {
	Path::Path(const std::string& path) : data(path) {
		for (auto& c : data)
            if (c == '\\')
                c = '/';
	}

	Path::Path(const char* path) : data{path} {
		for (auto& c : data)
			if (c == '\\')
				c = '/';
	}

	std::string Path::platform_path() const {
		std::string path = parse_prefix();
		// for (auto& c : path)
  //           if (c == '/')
  //               c = '\\';
		return path;
	}

	Path Path::from_platform_path(const std::string& path) {
		Path res{path};
		for (auto& c : res.data)
			if (c == '\\')
				c = '/';
		return res;
	}

    Path FileIO::exe_dir() {
	    char buff[MAX_PATH]{};
		constexpr size_t len = sizeof(buff);
	    const int bytes = GetModuleFileName(nullptr, buff, len);
		if (!bytes)
			return "";
		for (int i = bytes - 1; i >= 0; --i) {
			if (buff[i] == '\\') {
				buff[i] = '\0';
				break;
			}
		}
		std::string str{buff};
		for (auto& c : str)
			if (c == '\\')
				c = '/';
		return str;
    }
}
