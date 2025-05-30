#pragma once
#include "X11/X.h"
#include <chrono>
#include <cstdint>
#include <X11/Xlib.h>
#include <X11/Xutil.h>


namespace mgm {
    struct NativeWindow {
        Display* display = nullptr;
        Screen* screen = nullptr;
        int screenid = 0;
        Window window{};
        Atom wm_destroy{}, wm_hints{};
        uint32_t functions = 0x0;
        std::chrono::steady_clock::time_point last_mouse_query_time{};
    };
} // namespace mgm
