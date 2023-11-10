#pragma once
#include <windows.h>


namespace mgm {
    struct NativeWindow {
        HINSTANCE hinstance{};
	    HWND window{};
    };
}
