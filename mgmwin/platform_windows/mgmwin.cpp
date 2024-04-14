#include "mgmwin.hpp"
#include "native_window.hpp"
#include "mgmath.hpp"
#include <iostream>


namespace mgm {
	MgmWindow::MgmWindow(const char* name, vec2u32 size, Mode mode, vec2i32 pos) : log{std::string("Window ") + name} {
		input_interfaces = new float[(size_t)InputInterface::_NUM_INPUT_INTERFACES];
		open(name, size, mode, pos);
	}

	void MgmWindow::open(const char* name, vec2u32 size, Mode mode, vec2i32 pos) {
		data = new NativeWindow{};
		data->hinstance = GetModuleHandle(nullptr);

		WNDCLASS wc{};
		wc.style = CS_OWNDC;
		wc.lpfnWndProc = NativeWindow::window_proc;
		wc.hInstance = data->hinstance;
		wc.lpszClassName = "MgmWindowClass";

		RegisterClass(&wc);

		DWORD style{};
		switch (mode) {
			case Mode::NORMAL: {
				style = WS_OVERLAPPEDWINDOW;
				break;
			}
			case Mode::BORDERLESS: {
				style = WS_POPUP;
				break;
			}
			case Mode::FULLSCREEN: {
				style = WS_MAXIMIZE | WS_POPUP;
				break;
			}
		}
		window_mode = mode;

		data->window = CreateWindowEx(
			0,
			"MgmWindowClass",
			name,
			style,
			pos.x(), pos.y(),
			size.x(), size.y(),
			nullptr,
			nullptr,
			data->hinstance,
			this
		);

		if (data->window == nullptr)
			return;

		ShowWindow(data->window, SW_SHOW);
		set_size(size);
		set_position(pos);

		window_pos = pos;
		window_size = size;

		_is_open = true;
	}

	void MgmWindow::set_mode(const Mode mode) {
		log.warning("Modifying window style on Windows is considered unstable");
		if (!_is_open)
			return;
		DWORD style{};
		switch (mode) {
			case Mode::NORMAL: {
				style = WS_OVERLAPPEDWINDOW;
				break;
			}
			case Mode::BORDERLESS: {
				style = WS_POPUP;
				break;
			}
			case Mode::FULLSCREEN: {
				style = WS_MAXIMIZE | WS_POPUP;
				break;
			}
		}
		SetWindowLongPtr(data->window, GWL_STYLE, style);
		SetWindowLongPtr(data->window, GWL_EXSTYLE, 0);
		SetWindowPos(data->window, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
		ShowWindow(data->window, SW_SHOW);
		window_mode = mode;
	}

	void MgmWindow::set_allow_resize(const bool allow) {
		if (window_mode != Mode::NORMAL)
			return;

		const auto old = GetWindowLongPtr(data->window, GWL_STYLE);
		if (allow)
			SetWindowLongPtr(data->window, GWL_STYLE, old | WS_THICKFRAME);
		else
			SetWindowLongPtr(data->window, GWL_STYLE, old & ~WS_THICKFRAME);

		_allow_resize = allow;
	}

	void MgmWindow::set_allow_close(const bool allow) {
		if (!allow)
			log.warning("Disallowing close will also disallow force close from the API");
		_allow_close = allow;
	}

	void MgmWindow::set_allow_maximize(const bool allow) {
		if (window_mode != Mode::NORMAL)
			return;

		const auto old = GetWindowLongPtr(data->window, GWL_STYLE);
		if (allow)
			SetWindowLongPtr(data->window, GWL_STYLE, old | WS_MAXIMIZEBOX);
		else
			SetWindowLongPtr(data->window, GWL_STYLE, old & ~WS_MAXIMIZEBOX);

		_allow_maximize = allow;
	}

	void MgmWindow::set_allow_minimize(const bool allow) {
		if (window_mode != Mode::NORMAL)
			return;

		const auto old = GetWindowLongPtr(data->window, GWL_STYLE);
		if (allow)
			SetWindowLongPtr(data->window, GWL_STYLE, old | WS_MINIMIZEBOX);
		else
			SetWindowLongPtr(data->window, GWL_STYLE, old & ~WS_MINIMIZEBOX);

		_allow_maximize = allow;
	}

	void MgmWindow::set_size(vec2u32 size) {
		SetWindowPos(data->window, nullptr, window_pos.x(), window_pos.y(), static_cast<int>(size.x()), static_cast<int>(size.y()), 0);
		RECT rect{};
		GetClientRect(data->window, &rect);
		const vec2u32 actual_size = {
			size.x() + (size.x() - (rect.right - rect.left)),
			size.y() + (size.y() - (rect.bottom - rect.top)),
		};
		SetWindowPos(data->window, nullptr, window_pos.x(), window_pos.y(), actual_size.x(), actual_size.y(), 0);
		window_size = size;
	}

	void MgmWindow::set_position(const vec2i32 pos) {
		if (pos.x() < 0 || pos.y() < 0) {
		    POINT cursor{};
			GetCursorPos(&cursor);
			const vec2i32 target_pos = vec2i32{cursor.x, cursor.y} - vec2i32{static_cast<int>(window_size.x()), static_cast<int>(window_size.y())} / 2;
			SetWindowPos(data->window, nullptr, target_pos.x(), target_pos.y(), window_size.x(), window_size.y(), SWP_NOSIZE | SWP_NOZORDER);
			return;
		}
		SetWindowPos(data->window, nullptr, pos.x(), pos.y(), window_size.x(), window_size.y(), SWP_NOSIZE | SWP_NOZORDER);
		window_pos = pos;
	}

	void MgmWindow::close() {
		if (!_is_open)
			return;

		CloseWindow(data->window);
		delete data;
		data = nullptr;
		_is_open = false;

		log.log("Closed Window");
	}

	MgmWindow::InputInterface winkey_to_input_interface(const WPARAM key) {
		switch (key) {
			case VK_LWIN: case VK_RWIN: return MgmWindow::InputInterface::Key_META;
		    case VK_CAPITAL: return MgmWindow::InputInterface::Key_CAPSLOCK;
		    case VK_NUMLOCK: return MgmWindow::InputInterface::Key_NUMLOCK;
		    case VK_SCROLL: return MgmWindow::InputInterface::Key_SCROLLLOCK;
		    case VK_SPACE: return MgmWindow::InputInterface::Key_SPACE;
		    case VK_RETURN: return MgmWindow::InputInterface::Key_ENTER;
		    case VK_TAB: return MgmWindow::InputInterface::Key_TAB;
		    case VK_SHIFT: return MgmWindow::InputInterface::Key_SHIFT;
		    case VK_CONTROL: return MgmWindow::InputInterface::Key_CTRL;
		    case VK_MENU: return MgmWindow::InputInterface::Key_ALT;
		    case VK_ESCAPE: return MgmWindow::InputInterface::Key_ESC;
		    case VK_BACK: return MgmWindow::InputInterface::Key_BACKSPACE;
		    case VK_DELETE: return MgmWindow::InputInterface::Key_DELETE;
		    case VK_INSERT: return MgmWindow::InputInterface::Key_INSERT;
		    case VK_HOME: return MgmWindow::InputInterface::Key_HOME;
		    case VK_END: return MgmWindow::InputInterface::Key_END;
		    case VK_PRIOR: return MgmWindow::InputInterface::Key_PAGEUP;
		    case VK_NEXT: return MgmWindow::InputInterface::Key_PAGEDOWN;
		    case VK_UP: return MgmWindow::InputInterface::Key_ARROW_UP;
		    case VK_DOWN: return MgmWindow::InputInterface::Key_ARROW_DOWN;
		    case VK_LEFT: return MgmWindow::InputInterface::Key_ARROW_LEFT;
		    case VK_RIGHT: return MgmWindow::InputInterface::Key_ARROW_RIGHT;
		    case VK_F1: return MgmWindow::InputInterface::Key_F1;
		    case VK_F2: return MgmWindow::InputInterface::Key_F2;
		    case VK_F3: return MgmWindow::InputInterface::Key_F3;
		    case VK_F4: return MgmWindow::InputInterface::Key_F4;
		    case VK_F5: return MgmWindow::InputInterface::Key_F5;
		    case VK_F6: return MgmWindow::InputInterface::Key_F6;
		    case VK_F7: return MgmWindow::InputInterface::Key_F7;
		    case VK_F8: return MgmWindow::InputInterface::Key_F8;
		    case VK_F9: return MgmWindow::InputInterface::Key_F9;
		    case VK_F10: return MgmWindow::InputInterface::Key_F10;
		    case VK_F11: return MgmWindow::InputInterface::Key_F11;
		    case VK_F12: return MgmWindow::InputInterface::Key_F12;
			default: return MgmWindow::InputInterface::NONE;
		}
	}

	MgmWindow::InputInterface winchar_to_input_interface(const WPARAM key) {
		switch (key) {
			case 'A': case 'a': return MgmWindow::InputInterface::Key_A;
			case 'B': case 'b': return MgmWindow::InputInterface::Key_B;
		    case 'C': case 'c': return MgmWindow::InputInterface::Key_C;
		    case 'D': case 'd': return MgmWindow::InputInterface::Key_D;
		    case 'E': case 'e': return MgmWindow::InputInterface::Key_E;
		    case 'F': case 'f': return MgmWindow::InputInterface::Key_F;
		    case 'G': case 'g': return MgmWindow::InputInterface::Key_G;
		    case 'H': case 'h': return MgmWindow::InputInterface::Key_H;
		    case 'I': case 'i': return MgmWindow::InputInterface::Key_I;
		    case 'J': case 'j': return MgmWindow::InputInterface::Key_J;
		    case 'K': case 'k': return MgmWindow::InputInterface::Key_K;
		    case 'L': case 'l': return MgmWindow::InputInterface::Key_L;
		    case 'M': case 'm': return MgmWindow::InputInterface::Key_M;
		    case 'N': case 'n': return MgmWindow::InputInterface::Key_N;
		    case 'O': case 'o': return MgmWindow::InputInterface::Key_O;
		    case 'P': case 'p': return MgmWindow::InputInterface::Key_P;
		    case 'Q': case 'q': return MgmWindow::InputInterface::Key_Q;
		    case 'R': case 'r': return MgmWindow::InputInterface::Key_R;
		    case 'S': case 's': return MgmWindow::InputInterface::Key_S;
		    case 'T': case 't': return MgmWindow::InputInterface::Key_T;
		    case 'U': case 'u': return MgmWindow::InputInterface::Key_U;
		    case 'V': case 'v': return MgmWindow::InputInterface::Key_V;
		    case 'W': case 'w': return MgmWindow::InputInterface::Key_W;
		    case 'X': case 'x': return MgmWindow::InputInterface::Key_X;
		    case 'Y': case 'y': return MgmWindow::InputInterface::Key_Y;
		    case 'Z': case 'z': return MgmWindow::InputInterface::Key_Z;
		    case '0': return MgmWindow::InputInterface::Key_0;
		    case '1': return MgmWindow::InputInterface::Key_1;
		    case '2': return MgmWindow::InputInterface::Key_2;
		    case '3': return MgmWindow::InputInterface::Key_3;
		    case '4': return MgmWindow::InputInterface::Key_4;
		    case '5': return MgmWindow::InputInterface::Key_5;
		    case '6': return MgmWindow::InputInterface::Key_6;
		    case '7': return MgmWindow::InputInterface::Key_7;
		    case '8': return MgmWindow::InputInterface::Key_8;
		    case '9': return MgmWindow::InputInterface::Key_9;
			case '`': return MgmWindow::InputInterface::Key_GRAVE;
			case '~': return MgmWindow::InputInterface::Key_TILDE;
		    case '!': return MgmWindow::InputInterface::Key_EXCLAMATION_MARK;
		    case '@': return MgmWindow::InputInterface::Key_AT;
		    case '#': return MgmWindow::InputInterface::Key_HASH;
		    case '$': return MgmWindow::InputInterface::Key_DOLLAR;
		    case '%': return MgmWindow::InputInterface::Key_PERCENT;
		    case '^': return MgmWindow::InputInterface::Key_CARET;
		    case '&': return MgmWindow::InputInterface::Key_AMPERSAND;
		    case '*': return MgmWindow::InputInterface::Key_ASTERISK;
		    case '(': return MgmWindow::InputInterface::Key_OPEN_PARENTHESIS;
		    case ')': return MgmWindow::InputInterface::Key_CLOSE_PARENTHESIS;
		    case '_': return MgmWindow::InputInterface::Key_UNDERSCORE;
		    case '+': return MgmWindow::InputInterface::Key_PLUS;
		    case '-': return MgmWindow::InputInterface::Key_MINUS;
		    case '=': return MgmWindow::InputInterface::Key_EQUAL;
		    case '[': return MgmWindow::InputInterface::Key_OPEN_BRACKET;
		    case ']': return MgmWindow::InputInterface::Key_CLOSE_BRACKET;
		    case '\\': return MgmWindow::InputInterface::Key_BACKSLASH;
		    case '{': return MgmWindow::InputInterface::Key_OPEN_CURLY_BRACKET;
		    case '}': return MgmWindow::InputInterface::Key_CLOSE_CURLY_BRACKET;
		    case '|': return MgmWindow::InputInterface::Key_VERTICAL_LINE;
		    case ';': return MgmWindow::InputInterface::Key_SEMICOLON;
		    case '\'': return MgmWindow::InputInterface::Key_APOSTROPHE;
		    case ':': return MgmWindow::InputInterface::Key_COLON;
		    case '"': return MgmWindow::InputInterface::Key_QUOTE;
		    case ',': return MgmWindow::InputInterface::Key_COMMA;
		    case '.': return MgmWindow::InputInterface::Key_PERIOD;
		    case '/': return MgmWindow::InputInterface::Key_FORWARD_SLASH;
		    case '<': return MgmWindow::InputInterface::Key_LESS;
		    case '>': return MgmWindow::InputInterface::Key_GREATER;
		    case '?': return MgmWindow::InputInterface::Key_QUESTION_MARK;
		    case ' ': return MgmWindow::InputInterface::Key_SPACE;
			case '\t': return MgmWindow::InputInterface::Key_TAB;
			default: return MgmWindow::InputInterface::NONE;
		}
	}

	thread_local MgmWindow* updating_window = nullptr;

	LRESULT CALLBACK NativeWindow::window_proc(HWND h_window, UINT u_msg, WPARAM w_param, LPARAM l_param) {
		switch (u_msg) {
			case WM_SIZE: {
				if (updating_window != nullptr) {
					RECT rect{};
					GetClientRect(updating_window->data->window, &rect);
					updating_window->window_size = {LOWORD(l_param), HIWORD(l_param)};
				}
				return LRESULT{};
			}
			case WM_CLOSE: {
				if (updating_window != nullptr)
					if (updating_window->get_allow_close())
						updating_window->set_should_close_next_update();
				return LRESULT{};
			}
			case WM_KEYDOWN: {
				const auto key = winkey_to_input_interface(w_param);
				updating_window->input_events_since_last_update.emplace_back(
					MgmWindow::InputEvent{key, 1.0f, MgmWindow::InputEvent::Mode::PRESS, MgmWindow::InputEvent::From::KEYBOARD}
				);
				updating_window->get_input_interface(key) = 1.0f;
				break;
			}
			case WM_KEYUP: {
				const auto key = winkey_to_input_interface(w_param);
				updating_window->input_events_since_last_update.emplace_back(
					MgmWindow::InputEvent{key, 0.0f, MgmWindow::InputEvent::Mode::RELEASE, MgmWindow::InputEvent::From::KEYBOARD}
				);
				updating_window->get_input_interface(key) = 0.0f;
				break;
			}
			case WM_CHAR: {
				updating_window->text_input_since_last_update += static_cast<char>(w_param);
				break;
			}
			case WM_LBUTTONDOWN: {
				updating_window->input_events_since_last_update.emplace_back(
					MgmWindow::InputEvent{MgmWindow::InputInterface::Mouse_LEFT, 1.0f, MgmWindow::InputEvent::Mode::PRESS, MgmWindow::InputEvent::From::MOUSE}
				);
				updating_window->get_input_interface(MgmWindow::InputInterface::Mouse_LEFT) = 1.0f;
				break;
			}
			case WM_LBUTTONUP: {
				updating_window->input_events_since_last_update.emplace_back(
					MgmWindow::InputEvent{MgmWindow::InputInterface::Mouse_LEFT, 0.0f, MgmWindow::InputEvent::Mode::RELEASE, MgmWindow::InputEvent::From::MOUSE}
				);
				updating_window->get_input_interface(MgmWindow::InputInterface::Mouse_LEFT) = 0.0f;
			    break;
			}
			case WM_RBUTTONDOWN: {
				updating_window->input_events_since_last_update.emplace_back(
					MgmWindow::InputEvent{MgmWindow::InputInterface::Mouse_RIGHT, 1.0f, MgmWindow::InputEvent::Mode::PRESS, MgmWindow::InputEvent::From::MOUSE}
				);
				updating_window->get_input_interface(MgmWindow::InputInterface::Mouse_RIGHT) = 1.0f;
			    break;
			}
			case WM_RBUTTONUP: {
				updating_window->input_events_since_last_update.emplace_back(
					MgmWindow::InputEvent{MgmWindow::InputInterface::Mouse_RIGHT, 0.0f, MgmWindow::InputEvent::Mode::RELEASE, MgmWindow::InputEvent::From::MOUSE}
				);
				updating_window->get_input_interface(MgmWindow::InputInterface::Mouse_RIGHT) = 0.0f;
			    break;
			}
			case WM_MBUTTONDOWN: {
				updating_window->input_events_since_last_update.emplace_back(
					MgmWindow::InputEvent{MgmWindow::InputInterface::Mouse_MIDDLE, 1.0f, MgmWindow::InputEvent::Mode::PRESS, MgmWindow::InputEvent::From::MOUSE}
				);
				updating_window->get_input_interface(MgmWindow::InputInterface::Mouse_MIDDLE) = 1.0f;
			    break;
			}
			case WM_MBUTTONUP: {
				updating_window->input_events_since_last_update.emplace_back(
					MgmWindow::InputEvent{MgmWindow::InputInterface::Mouse_MIDDLE, 0.0f, MgmWindow::InputEvent::Mode::RELEASE, MgmWindow::InputEvent::From::MOUSE}
				);
				updating_window->get_input_interface(MgmWindow::InputInterface::Mouse_MIDDLE) = 0.0f;
			    break;
			}
			case WM_MOUSEWHEEL: {
				const auto delta = GET_WHEEL_DELTA_WPARAM(w_param);
				if (delta > 0) {
					updating_window->input_events_since_last_update.emplace_back(
						MgmWindow::InputEvent{MgmWindow::InputInterface::Mouse_SCROLL_UP, 1.0f, MgmWindow::InputEvent::Mode::PRESS, MgmWindow::InputEvent::From::MOUSE}
					);
					updating_window->get_input_interface(MgmWindow::InputInterface::Mouse_SCROLL_UP) = 1.0f;
				}
				else if (delta < 0) {
					updating_window->input_events_since_last_update.emplace_back(
						MgmWindow::InputEvent{MgmWindow::InputInterface::Mouse_SCROLL_DOWN, 1.0f, MgmWindow::InputEvent::Mode::PRESS, MgmWindow::InputEvent::From::MOUSE}
					);
					updating_window->get_input_interface(MgmWindow::InputInterface::Mouse_SCROLL_DOWN) = 1.0f;
				}
				break;
			}
			case WM_MOUSEMOVE: {
			    const vec2i32 pos = {LOWORD(l_param), HIWORD(l_param)};
				const vec2f posf = vec2f{static_cast<float>(pos.x()), static_cast<float>(pos.y())}
				    / vec2f{static_cast<float>(updating_window->window_size.x()), static_cast<float>(updating_window->window_size.y())} * 2.0f - 1.0f;
				updating_window->get_input_interface(MgmWindow::InputInterface::Mouse_POS_X) = posf.x();
				updating_window->get_input_interface(MgmWindow::InputInterface::Mouse_POS_Y) = posf.y();
				updating_window->input_events_since_last_update.emplace_back(
					MgmWindow::InputInterface::Mouse_POS_X,
					posf.x(),
					MgmWindow::InputEvent::Mode::OTHER,
					MgmWindow::InputEvent::From::MOUSE
				);
                updating_window->input_events_since_last_update.emplace_back(
                    MgmWindow::InputInterface::Mouse_POS_Y,
                    posf.y(),
                    MgmWindow::InputEvent::Mode::OTHER,
                    MgmWindow::InputEvent::From::MOUSE
                );
				break;
			}
			default: {
				return DefWindowProc(h_window, u_msg, w_param, l_param);
			}
		}
		return LRESULT{};
	}

	void MgmWindow::update() {
		updating_window = this;

		POINT cursor{};
		GetCursorPos(&cursor);
		ScreenToClient(data->window, &cursor);
		get_input_interface(InputInterface::Mouse_POS_X) = static_cast<float>(cursor.x) / static_cast<float>(window_size.x()) * 2.0f - 1.0f;
		get_input_interface(InputInterface::Mouse_POS_Y) = static_cast<float>(cursor.y) / static_cast<float>(window_size.y()) * 2.0f - 1.0f;

		get_input_interface(InputInterface::Mouse_SCROLL_UP) = 0.0f;
		get_input_interface(InputInterface::Mouse_SCROLL_DOWN) = 0.0f;

		input_events_since_last_update.clear();
		text_input_since_last_update.clear();

		MSG msg;
        while (PeekMessage(&msg, data->window, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_CLOSE && _allow_close == false)
				continue;

			TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
		updating_window = nullptr;

		if (_should_close)
			close();
	}

	MgmWindow::~MgmWindow() {
		delete input_interfaces;
		close();
	}
}
