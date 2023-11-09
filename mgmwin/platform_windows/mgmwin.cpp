#include "mgmwin.hpp"
#include "native_window.hpp"
#include "mgmath.hpp"


namespace mgm {
	struct MgmWindow::WindowData {
		HINSTANCE hinstance{};
		HWND window{};
	};

	LRESULT CALLBACK window_proc(HWND h_window, UINT u_msg, WPARAM w_param, LPARAM l_param) {
		// const auto* create_struct = reinterpret_cast<CREATESTRUCT*>(l_param);
		// MgmWindow* window = nullptr;
		// if (create_struct != nullptr && w_param == 0)
		// 	window = static_cast<MgmWindow*>(create_struct->lpCreateParams);
		switch (u_msg) {
			case WM_SIZE: {
				const vec2u32 size{LOWORD(l_param), HIWORD(l_param)};
				break;
			}
			case WM_DESTROY: {
				break;
			}
			default: {
				break;
			}
		}
		return DefWindowProc(h_window, u_msg, w_param, l_param);
	}

	NativeWindow* MgmWindow::get_native_window() {
		return native_window_data_copy;
	}

	MgmWindow::MgmWindow(const char* name, vec2i32 pos, vec2u32 size, Mode mode) : log{std::string("Window ") + name} {
		open(name, pos, size, mode);
	}

	void MgmWindow::open(const char* name, vec2i32 pos, vec2u32 size, Mode mode) {
		data = new WindowData{};
		data->hinstance = GetModuleHandle(nullptr);

		native_window_data_copy = new NativeWindow{};

		WNDCLASS wc{};
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
		if (_is_open)
			return;

		CloseWindow(data->window);
		delete data;
		data = nullptr;
		delete native_window_data_copy;
		native_window_data_copy = nullptr;
		_is_open = false;
	}

	void MgmWindow::update() {
	}

	MgmWindow::~MgmWindow() {
		close();
	}
}
