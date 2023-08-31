#include "mgmwin.hpp"

#include "logging.hpp"

#if defined(__linux__)
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#elif defined(WIN32)
#endif
#include <string>



namespace mgm {
    #if defined(__linux__)
    struct MgmWindow::WindowData {
        Display* display = nullptr;
        Screen* screen = nullptr;
        int screenid = 0;
        Window window{};
        Atom wm_destroy{}, wm_hints{};
        uint32_t functions = 0x0;
    };

    constexpr uint32_t WM_HINT_FUNCTIONS = 0x1,
        WM_HINT_BORDER = 0x2,
        WM_HINT_INPUT_MODE = 0x3,
        WM_HINT_INPUT_STATUS = 0x4;
    struct MwmHints {
        unsigned long flags = 0;
        // 0(no resize + no close), 1(), 2(no close), 3(no resize)
        unsigned long functions = 0;
        // 0(off), 1(on)
        unsigned long border = 0;
        // TODO
        long input_mode = 0;
        // TODO
        unsigned long status = 0;
    };

    void mgmwin_xwindow_hints(Display* display, Window window, Atom wm_hints, MwmHints* hints) {
        XChangeProperty(display, window, wm_hints, wm_hints, 32, PropModeReplace, (unsigned char*)hints, 5);
    }


    void* MgmWindow::get_native_display() {
        return data->display;
    }

    uint32_t MgmWindow::get_native_window() {
        return data->window;
    }

    MgmWindow::MgmWindow(const char* name, vec2i32 pos, vec2u32 size, Mode mode):
    log{(std::string("Window \"") + name + '\"').c_str()} {
        open(name, pos, size, mode);
    }

    void MgmWindow::open(const char* name, vec2i32 pos, vec2u32 size, Mode mode) {
        if (_is_open)
            close();

        data = new WindowData();
        data->display = XOpenDisplay(nullptr);
        if (!data->display) {
            log.error("Could not open X display");
            return;
        }
        log.log("Opened X display");

        data->screenid = DefaultScreen(data->display);
        data->window = XCreateSimpleWindow(
            data->display,
            RootWindow(data->display, data->screenid),
            0, 0,
            size.x(), size.y(),
            0,
            WhitePixel(data->display, data->screenid),
            BlackPixel(data->display, data->screenid)
        );
        XSetStandardProperties(data->display, data->window, name, nullptr, None, nullptr, 0, nullptr);
        window_size = size;
        log.log("Opened empty X window");

        XSelectInput(
            data->display, data->window,
            StructureNotifyMask
            | ButtonPressMask | ButtonReleaseMask
            | PointerMotionMask
            | KeyPressMask | KeyReleaseMask
            | KeymapStateMask
        );

        data->wm_destroy = XInternAtom(data->display, "WM_DELETE_WINDOW", True);
        data->wm_hints = XInternAtom(data->display, "_MOTIF_WM_HINTS", True);
        XSetWMProtocols(data->display, data->window, &data->wm_destroy, 1);
        
        XClearWindow(data->display, data->window);
        XMapWindow(data->display, data->window);
        set_position(pos);

        _is_open = true;
        _should_close = false;

        log.log("X Window creation successful");

        set_mode(mode);
    }

    void MgmWindow::set_mode(Mode mode) {
        if (window_mode == Mode::FULLSCREEN) {
            window_mode = mode;
            set_size(window_size);
            set_position(window_pos);
        }

        switch (mode) {
            case Mode::NORMAL: {
                MwmHints hints {
                    .flags = WM_HINT_BORDER,
                    .border = 1
                };
                mgmwin_xwindow_hints(data->display, data->window, data->wm_hints, &hints);
                log.log("Made window normal");
                break;
            }
            case Mode::BORDERLESS: {
                MwmHints hints {
                    .flags = WM_HINT_BORDER,
                    .border = 0
                };
                mgmwin_xwindow_hints(data->display, data->window, data->wm_hints, &hints);
                log.log("Made window borderless");
                break;
            }
            case Mode::FULLSCREEN: {
                vec2u32 new_size{};
                new_size.x() = DisplayWidth(data->display, data->screenid);
                new_size.y() = DisplayHeight(data->display, data->screenid);

                set_mode(Mode::BORDERLESS);
                window_mode = mode;
                set_position(vec2i32(0, 0));
                set_size(new_size);
                log.log("Made window fullscreen");
                break;
            }
            default:
                break;
        }
        window_mode = mode;
        if (mode != Mode::FULLSCREEN)
            nonfullscreen_window_mode = mode;
    }

    void MgmWindow::set_allow_resize(bool allow) {
        _allow_resize = allow;
        data->functions = (!_allow_resize * 2) | _allow_close;
        MwmHints hints{
            .flags = WM_HINT_FUNCTIONS,
            .functions = data->functions
        };
        mgmwin_xwindow_hints(data->display, data->window, data->wm_hints, &hints);
    }

    void MgmWindow::set_allow_close(bool allow) {
        _allow_close = allow;
        data->functions = (!_allow_resize * 2) | _allow_close;
        MwmHints hints{
            .flags = WM_HINT_FUNCTIONS,
            .functions = data->functions
        };
        mgmwin_xwindow_hints(data->display, data->window, data->wm_hints, &hints);
    }

    void MgmWindow::set_size(vec2u32 size) {
        XResizeWindow(data->display, data->window, size.x(), size.y());
        if (window_mode != Mode::FULLSCREEN)
            window_size = size;
    }

    void MgmWindow::set_position(vec2i32 pos) {
        if (pos.x() < 0)
            pos.x() = (DisplayWidth(data->display, data->screenid) - window_size.x()) / 2;
        if (pos.y() < 0)
            pos.y() = (DisplayHeight(data->display, data->screenid) - window_size.y()) / 2;
        XMoveWindow(data->display, data->window, pos.x(), pos.y());
        if (window_mode != Mode::FULLSCREEN)
            window_pos = pos;
    }

    void MgmWindow::close() {
        if (!_is_open)
            return;

        XDestroyWindow(data->display, data->window);
        XCloseDisplay(data->display);
        _is_open = false;
        log.log("Closed window");
        memset(data, 0, sizeof(WindowData));
    }

    void MgmWindow::update() {
        if (!_is_open)
            return;

        XEvent event{};
        while (XEventsQueued(data->display, QueuedAfterFlush)) {
            XNextEvent(data->display, &event);
            switch (event.type) {
                case ClientMessage: {
                    if (event.xclient.data.l[0] == data->wm_destroy)
                        _should_close = true;
                    break;
                }
                default:
                    break;
            }
        }

        if (_should_close)
            close();
    }

    MgmWindow::~MgmWindow() {
        close();
        delete data;
    }



    #elif defined(_WIN32)
    struct MgmWindow::WindowData {
        
    }
    #endif
}
