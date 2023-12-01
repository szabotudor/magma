#pragma once
#include <windows.h>


namespace mgm {
    struct NativeWindow {
        HINSTANCE hinstance{};
	    HWND window{};
        static LRESULT CALLBACK window_proc(HWND h_window, UINT u_msg, WPARAM w_param, LPARAM l_param);
    };
}
