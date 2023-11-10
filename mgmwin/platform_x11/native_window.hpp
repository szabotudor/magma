#pragma once
#include "X11/X.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cstdint>


namespace mgm {
    struct NativeWindow {
        Display* display = nullptr;
        Screen* screen = nullptr;
        int screenid = 0;
        Window window{};
        Atom wm_destroy{}, wm_hints{};
        uint32_t functions = 0x0;
    };
}
