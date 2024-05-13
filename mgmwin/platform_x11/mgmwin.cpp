#include "mgmwin.hpp"
#include "mgmath.hpp"
#include "native_window.hpp"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <bits/chrono.h>
#include <chrono>
#include <iostream>
#include <ratio>
#include <string>


namespace mgm {
    constexpr uint32_t WM_HINT_FUNCTIONS = 0x1,
        WM_HINT_BORDER = 0x2;
        // Unused for now
        // WM_HINT_INPUT_MODE = 0x3,
        // WM_HINT_INPUT_STATUS = 0x4;
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


    void MgmWindow::open(const char* name, vec2u32 size, Mode mode, vec2i32 pos) {
        if (_is_open)
            close();

        data = new NativeWindow{};
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

    void MgmWindow::set_allow_maximize(const bool allow) {
        set_allow_resize(allow);
        _allow_maximize = allow;
    }

    void MgmWindow::set_allow_minimize(const bool) {
        log.error("X11 doesn't support blocking/allowing minimization");
    }

    void MgmWindow::set_size(vec2u32 size) {
        XResizeWindow(data->display, data->window, size.x(), size.y());
        if (window_mode != Mode::FULLSCREEN)
            window_size = size;
    }

    void MgmWindow::set_position(vec2i32 pos) {
        if (pos.x() < 0 || pos.y() < 0) {
            const auto screen_count = ScreenCount(data->display);
            vec2i32 mouse_pos{};
            vec2i32 root_mouse_pos{};
            vec2i32 screen_size{};
            Window root_window{};
            for (int i = 0; i < screen_count; i++) {
                root_window = RootWindow(data->display, i);
                Window current_window_return, root_window_return;
                uint32_t mask;
                XQueryPointer(data->display, root_window, &root_window_return, &current_window_return, &root_mouse_pos.x(), &root_mouse_pos.y(), &mouse_pos.x(), &mouse_pos.y(), &mask);
                screen_size = {DisplayWidth(data->display, i), DisplayHeight(data->display, i)};
                if (mouse_pos.x() >= 0 && mouse_pos.y() >= 0 && mouse_pos.x() < screen_size.x() && mouse_pos.y() < screen_size.y()) {
                    const auto half_size = vec2i32{static_cast<int>(window_size.x()), static_cast<int>(window_size.y())} / 2;
                    set_position(vec2i32::max(mouse_pos - half_size, vec2i32{0, 0}));
                    return;
                }
            }

            set_position(vec2i32{0, 0});
            return;
        }
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
        delete data;
        data = nullptr;
    }

    MgmWindow::InputInterface convert_x11_key(KeySym keysym) {
        switch (keysym) {
            case XK_A: case XK_a: return MgmWindow::InputInterface::Key_A;
            case XK_B: case XK_b: return MgmWindow::InputInterface::Key_B;
            case XK_C: case XK_c: return MgmWindow::InputInterface::Key_C;
            case XK_D: case XK_d: return MgmWindow::InputInterface::Key_D;
            case XK_E: case XK_e: return MgmWindow::InputInterface::Key_E;
            case XK_F: case XK_f: return MgmWindow::InputInterface::Key_F;
            case XK_G: case XK_g: return MgmWindow::InputInterface::Key_G;
            case XK_H: case XK_h: return MgmWindow::InputInterface::Key_H;
            case XK_I: case XK_i: return MgmWindow::InputInterface::Key_I;
            case XK_J: case XK_j: return MgmWindow::InputInterface::Key_J;
            case XK_K: case XK_k: return MgmWindow::InputInterface::Key_K;
            case XK_L: case XK_l: return MgmWindow::InputInterface::Key_L;
            case XK_M: case XK_m: return MgmWindow::InputInterface::Key_M;
            case XK_N: case XK_n: return MgmWindow::InputInterface::Key_N;
            case XK_O: case XK_o: return MgmWindow::InputInterface::Key_O;
            case XK_P: case XK_p: return MgmWindow::InputInterface::Key_P;
            case XK_Q: case XK_q: return MgmWindow::InputInterface::Key_Q;
            case XK_R: case XK_r: return MgmWindow::InputInterface::Key_R;
            case XK_S: case XK_s: return MgmWindow::InputInterface::Key_S;
            case XK_T: case XK_t: return MgmWindow::InputInterface::Key_T;
            case XK_U: case XK_u: return MgmWindow::InputInterface::Key_U;
            case XK_V: case XK_v: return MgmWindow::InputInterface::Key_V;
            case XK_W: case XK_w: return MgmWindow::InputInterface::Key_W;
            case XK_X: case XK_x: return MgmWindow::InputInterface::Key_X;
            case XK_Y: case XK_y: return MgmWindow::InputInterface::Key_Y;
            case XK_Z: case XK_z: return MgmWindow::InputInterface::Key_Z;
            case XK_0: return MgmWindow::InputInterface::Key_0;
            case XK_1: return MgmWindow::InputInterface::Key_1;
            case XK_2: return MgmWindow::InputInterface::Key_2;
            case XK_3: return MgmWindow::InputInterface::Key_3;
            case XK_4: return MgmWindow::InputInterface::Key_4;
            case XK_5: return MgmWindow::InputInterface::Key_5;
            case XK_6: return MgmWindow::InputInterface::Key_6;
            case XK_7: return MgmWindow::InputInterface::Key_7;
            case XK_8: return MgmWindow::InputInterface::Key_8;
            case XK_9: return MgmWindow::InputInterface::Key_9;
            case XK_Meta_L: case XK_Meta_R: return MgmWindow::InputInterface::Key_META;
            case XK_Caps_Lock: return MgmWindow::InputInterface::Key_CAPSLOCK;
            case XK_Num_Lock: return MgmWindow::InputInterface::Key_NUMLOCK;
            case XK_Scroll_Lock: return MgmWindow::InputInterface::Key_SCROLLLOCK;
            case XK_space: return MgmWindow::InputInterface::Key_SPACE;
            case XK_Return: return MgmWindow::InputInterface::Key_ENTER;
            case XK_Tab: return MgmWindow::InputInterface::Key_TAB;
            case XK_Shift_L: case XK_Shift_R: return MgmWindow::InputInterface::Key_SHIFT;
            case XK_Control_L: case XK_Control_R: return MgmWindow::InputInterface::Key_CTRL;
            case XK_Alt_L: case XK_Alt_R: return MgmWindow::InputInterface::Key_ALT;
            case XK_Escape: return MgmWindow::InputInterface::Key_ESC;
            case XK_BackSpace: return MgmWindow::InputInterface::Key_BACKSPACE;
            case XK_Delete: return MgmWindow::InputInterface::Key_DELETE;
            case XK_Insert: return MgmWindow::InputInterface::Key_INSERT;
            case XK_Home: return MgmWindow::InputInterface::Key_HOME;
            case XK_End: return MgmWindow::InputInterface::Key_END;
            case XK_Page_Up: return MgmWindow::InputInterface::Key_PAGEUP;
            case XK_Page_Down: return MgmWindow::InputInterface::Key_PAGEDOWN;
            case XK_Up: return MgmWindow::InputInterface::Key_ARROW_UP;
            case XK_Down: return MgmWindow::InputInterface::Key_ARROW_DOWN;
            case XK_Left: return MgmWindow::InputInterface::Key_ARROW_LEFT;
            case XK_Right: return MgmWindow::InputInterface::Key_ARROW_RIGHT;
            case XK_F1: return MgmWindow::InputInterface::Key_F1;
            case XK_F2: return MgmWindow::InputInterface::Key_F2;
            case XK_F3: return MgmWindow::InputInterface::Key_F3;
            case XK_F4: return MgmWindow::InputInterface::Key_F4;
            case XK_F5: return MgmWindow::InputInterface::Key_F5;
            case XK_F6: return MgmWindow::InputInterface::Key_F6;
            case XK_F7: return MgmWindow::InputInterface::Key_F7;
            case XK_F8: return MgmWindow::InputInterface::Key_F8;
            case XK_F9: return MgmWindow::InputInterface::Key_F9;
            case XK_F10: return MgmWindow::InputInterface::Key_F10;
            case XK_F11: return MgmWindow::InputInterface::Key_F11;
            case XK_F12: return MgmWindow::InputInterface::Key_F12;
            case XK_plus: return MgmWindow::InputInterface::Key_PLUS;
            case XK_minus: return MgmWindow::InputInterface::Key_MINUS;
            case XK_asterisk: return MgmWindow::InputInterface::Key_ASTERISK;
            case XK_slash: return MgmWindow::InputInterface::Key_FORWARD_SLASH;
            case XK_equal: return MgmWindow::InputInterface::Key_EQUAL;
            case XK_comma: return MgmWindow::InputInterface::Key_COMMA;
            case XK_period: return MgmWindow::InputInterface::Key_PERIOD;
            case XK_colon: return MgmWindow::InputInterface::Key_COLON;
            case XK_semicolon: return MgmWindow::InputInterface::Key_SEMICOLON;
            case XK_apostrophe: return MgmWindow::InputInterface::Key_APOSTROPHE;
            case XK_quotedbl: return MgmWindow::InputInterface::Key_QUOTE;
            case XK_braceleft: return MgmWindow::InputInterface::Key_OPEN_CURLY_BRACKET;
            case XK_braceright: return MgmWindow::InputInterface::Key_CLOSE_CURLY_BRACKET;
            case XK_bracketleft: return MgmWindow::InputInterface::Key_OPEN_BRACKET;
            case XK_bracketright: return MgmWindow::InputInterface::Key_CLOSE_BRACKET;
            case XK_backslash: return MgmWindow::InputInterface::Key_BACKSLASH;
            case XK_question: return MgmWindow::InputInterface::Key_QUESTION_MARK;
            case XK_exclam: return MgmWindow::InputInterface::Key_EXCLAMATION_MARK;
            case XK_at: return MgmWindow::InputInterface::Key_AT;
            case XK_numbersign: return MgmWindow::InputInterface::Key_HASH;
            case XK_dollar: return MgmWindow::InputInterface::Key_DOLLAR;
            case XK_percent: return MgmWindow::InputInterface::Key_PERCENT;
            case XK_asciicircum: return MgmWindow::InputInterface::Key_CARET;
            case XK_less: return MgmWindow::InputInterface::Key_LESS;
            case XK_greater: return MgmWindow::InputInterface::Key_GREATER;
            case XK_ampersand: return MgmWindow::InputInterface::Key_AMPERSAND;
            case XK_parenleft: return MgmWindow::InputInterface::Key_OPEN_PARENTHESIS;
            case XK_parenright: return MgmWindow::InputInterface::Key_CLOSE_PARENTHESIS;
            case XK_underscore: return MgmWindow::InputInterface::Key_UNDERSCORE;
            case XK_grave: return MgmWindow::InputInterface::Key_GRAVE;
            case XK_asciitilde: return MgmWindow::InputInterface::Key_TILDE;
            case XK_bar: return MgmWindow::InputInterface::Key_VERTICAL_LINE;
            default: return MgmWindow::InputInterface::NONE;
        }
    }

    MgmWindow::InputInterface convert_x11_mouse_button(uint32_t button) {
        switch (button) {
            case 1: return MgmWindow::InputInterface::Mouse_LEFT;
            case 2: return MgmWindow::InputInterface::Mouse_MIDDLE;
            case 3: return MgmWindow::InputInterface::Mouse_RIGHT;
            case 4: return MgmWindow::InputInterface::Mouse_SCROLL_UP;
            case 5: return MgmWindow::InputInterface::Mouse_SCROLL_DOWN;
            default: return MgmWindow::InputInterface::NONE;
        }
    }

    void MgmWindow::update() {
        if (!_is_open)
            return;

        input_interfaces[(size_t)InputInterface::Mouse_SCROLL_UP] = 0.0f;
        input_interfaces[(size_t)InputInterface::Mouse_SCROLL_DOWN] = 0.0f;

        const auto current_mouse_query_time = std::chrono::steady_clock::now();
        const auto mills = std::chrono::duration_cast<std::chrono::milliseconds>(current_mouse_query_time - data->last_mouse_query_time).count();
        if (mills > 1) {
            Window current_window, root_window;
            int rx, ry, x, y;
            uint32_t mask;
            XQueryPointer(data->display, data->window, &root_window, &current_window, &rx, &ry, &x, &y, &mask);
            input_interfaces[(size_t)InputInterface::Mouse_POS_X] = (float)x / (float)window_size.x() * 2.0f - 1.0f;
            input_interfaces[(size_t)InputInterface::Mouse_POS_Y] = (float)y / (float)window_size.y() * 2.0f - 1.0f;
            data->last_mouse_query_time = current_mouse_query_time;
        }

        input_events_since_last_update.clear();
        text_input_since_last_update.clear();

        memcpy(
            input_interfaces + (size_t)InputInterface::_NUM_INPUT_INTERFACES,
            input_interfaces,
            (size_t)InputInterface::_NUM_INPUT_INTERFACES * sizeof(float)
        );

        XEvent event{};
        while (XEventsQueued(data->display, QueuedAfterFlush)) {
            XNextEvent(data->display, &event);
            switch (event.type) {
                case KeyPress: {
                    const auto key = convert_x11_key(XkbKeycodeToKeysym(data->display, event.xkey.keycode, 0, 0));
                    if (key == InputInterface::NONE)
                        break;
                    input_interfaces[(size_t)key] = 1.0f;
                    input_events_since_last_update.emplace_back(InputEvent{key, 1.0f, InputEvent::Mode::PRESS, InputEvent::From::KEYBOARD});

                    char buffer[32];
                    KeySym keysym;
                    XLookupString(&event.xkey, buffer, sizeof(buffer), &keysym, nullptr);
                    text_input_since_last_update += buffer;
                    break;
                }
                case KeyRelease: {
                    const auto key = convert_x11_key(XkbKeycodeToKeysym(data->display, event.xkey.keycode, 0, 0));
                    if (key == InputInterface::NONE)
                        break;
                    input_interfaces[(size_t)key] = 0.0f;
                    input_events_since_last_update.emplace_back(InputEvent{key, 0.0f, InputEvent::Mode::RELEASE, InputEvent::From::KEYBOARD});
                    break;
                }
                case ButtonPress: {
                    const auto button = convert_x11_mouse_button(event.xbutton.button);
                    if (button == InputInterface::NONE)
                        break;
                    input_interfaces[(size_t)button] = 1.0f;
                    input_events_since_last_update.emplace_back(InputEvent{button, 1.0f, InputEvent::Mode::PRESS, InputEvent::From::MOUSE});
                    break;
                }
                case ButtonRelease: {
                    const auto button = convert_x11_mouse_button(event.xbutton.button);
                    if (button == InputInterface::Mouse_SCROLL_DOWN || button == InputInterface::Mouse_SCROLL_UP || button == InputInterface::NONE)
                        break;
                    input_interfaces[(size_t)button] = 0.0f;
                    input_events_since_last_update.emplace_back(InputEvent{button, 0.0f, InputEvent::Mode::RELEASE, InputEvent::From::MOUSE});
                    break;
                }
                case MotionNotify: {
                    input_interfaces[(size_t)InputInterface::Mouse_POS_X] = (float)event.xmotion.x / (float)window_size.x() * 2.0f - 1.0f;
                    input_interfaces[(size_t)InputInterface::Mouse_POS_Y] = (float)event.xmotion.y / (float)window_size.y() * 2.0f - 1.0f;
                    input_events_since_last_update.emplace_back(InputEvent{
                        InputInterface::Mouse_POS_X,
                        input_interfaces[(size_t)InputInterface::Mouse_POS_X],
                        InputEvent::Mode::OTHER,
                        InputEvent::From::MOUSE
                    });
                    input_events_since_last_update.emplace_back(InputEvent{
                        InputInterface::Mouse_POS_Y,
                        input_interfaces[(size_t)InputInterface::Mouse_POS_Y],
                        InputEvent::Mode::OTHER,
                        InputEvent::From::MOUSE
                    });
                    break;
                }
                case ClientMessage: {
                    if (event.xclient.data.l[0] == (int)data->wm_destroy)
                        _should_close = true;
                    break;
                }
                case ConfigureNotify: {
                    if (event.xconfigure.width != (int)window_size.x()
                        || event.xconfigure.height != (int)window_size.y()) {
                        window_size = vec2u32{(uint32_t)event.xconfigure.width, (uint32_t)event.xconfigure.height};
                    }
                }
                default:
                    break;
            }
        }

        for (const auto& event : input_events_since_last_update)
            for (auto& callback : input_callbacks(event.interface))
                callback(event);
    }

    MgmWindow::~MgmWindow() {
        close();
        delete data;
        delete input_interfaces;
    }
}
