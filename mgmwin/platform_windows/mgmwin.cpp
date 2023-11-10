#include "mgmwin.hpp"
#include "native_window.hpp"
#include "mgmath.hpp"


namespace mgm {
	struct MgmWindow::WindowData {
		HINSTANCE hinstance{};
		HWND window{};
	};


	thread_local MgmWindow* updating_window = nullptr;

	LRESULT CALLBACK window_proc(HWND h_window, UINT u_msg, WPARAM w_param, LPARAM l_param) {
		switch (u_msg) {
			case WM_SIZE: {
				const vec2u32 size{LOWORD(l_param), HIWORD(l_param)};
				return LRESULT{};
			}
			case WM_CLOSE: {
				if (updating_window != nullptr)
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
		data = new WindowData{};
		data->hinstance = GetModuleHandle(nullptr);

		native_window_data_copy = new NativeWindow{};

		WNDCLASS wc{};
		wc.style = CS_OWNDC;
		wc.lpfnWndProc = window_proc;
		wc.hInstance = data->hinstance;
		wc.lpszClassName = "MgmWindowClass";

		RegisterClass(&wc);

		data->window = CreateWindowEx(
			0,
			"MgmWindowClass",
			name,
			WS_OVERLAPPEDWINDOW,
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

		native_window_data_copy->window = data->window;
		_is_open = true;
	}

	void MgmWindow::set_size(vec2u32 size) {
		window_size = size;
		SetWindowPos(data->window, nullptr, window_pos.x(), window_pos.y(), static_cast<int>(size.x()), static_cast<int>(size.y()), 0);
	}

	void MgmWindow::close() {
		if (!_is_open)
			return;

		CloseWindow(data->window);
		delete data;
		data = nullptr;
		delete native_window_data_copy;
		native_window_data_copy = nullptr;
		_is_open = false;

		log.log("Closed Window");
	}

	void MgmWindow::update() {
		updating_window = this;
		MSG msg;
        while (PeekMessage(&msg, data->window, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_CLOSE)
                _should_close = true;
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
