#include "file.hpp"
#include <Windows.h>

namespace mgm {
    std::string FileIO::exe_dir() {
	    char buff[256]{};
		constexpr size_t len = sizeof(buff);
	    const int bytes = GetModuleFileName(nullptr, buff, len);
		if (!bytes)
			return "";
		return buff;
    }
}