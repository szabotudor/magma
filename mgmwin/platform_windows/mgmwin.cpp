#include "mgmwin.hpp"
#include "native_window.hpp"
#include "mgmath.hpp"


namespace mgm {
	thread_local MgmWindow* updating_window = nullptr;

	LRESULT CALLBACK window_proc(HWND h_window, UINT u_msg, WPARAM w_param, LPARAM l_param) {
		switch (u_msg) {
			case WM_SIZE: {
				const vec2u32 size{LOWORD(l_param), HIWORD(l_param)};
				return LRESULT{};
			}
			case WM_CLOSE: {
				if (updating_window != nullptr)
					if (updating_window->get_allow_close())
						updating_window->set_should_close_next_update();
				return LRESULT{};
			}
			default: {
				return DefWindowProc(h_window, u_msg, w_param, l_param);
			}
		}
	}

	MgmWindow::MgmWindow(const char* name, vec2i32 pos, vec2u32 size, Mode mode) : log{std::string("Window ") + name} {
		open(name, pos, size, mode);
	}

	void MgmWindow::open(const char* name, vec2i32 pos, vec2u32 size, Mode mode) {
		data = new NativeWindow{};
		data->hinstance = GetModuleHandle(nullptr);

		WNDCLASS wc{};
		wc.style = CS_OWNDC;
		wc.lpfnWndProc = window_proc;
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
		window_size = size;
	}

	void MgmWindow::set_position(const vec2i32 pos) {
		SetWindowPos(data->window, nullptr, pos.x(), pos.y(), window_size.x(), window_size.y(), 0);
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

	void MgmWindow::update() {
		updating_window = this;
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
		close();
	}
}
