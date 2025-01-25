#include "backend_OpenGL.hpp"
#include "native_window.hpp"
#include "glad/glad.h"
#include "wglext.h"
#include <minwindef.h>


namespace mgm {
	struct OpenGLPlatform::GLBackendData {
		HGLRC context{};
		HDC h_device{};
	};

	void* OpenGLPlatform::proc_address_getter(const char* name) {
		auto p = reinterpret_cast<void*>(wglGetProcAddress(name));

		if(reinterpret_cast<size_t>(p) <= 3 || (p == reinterpret_cast<void*>(-1)) ) {
			static HMODULE module = LoadLibraryA("opengl32.dll");
			p = reinterpret_cast<void*>(GetProcAddress(module, name));
		}

		return p;
	}


	OpenGLPlatform::OpenGLPlatform(const bool is_opengl_es) {
		if (is_opengl_es) {
			log.error("Windows doesn't support OpenGL ES");
			return;
		}
		data = new GLBackendData{};
	}

	void OpenGLPlatform::create_context(int ver_major, int ver_minor, struct NativeWindow* native_window) {
		constexpr PIXELFORMATDESCRIPTOR pfd{
			sizeof(PIXELFORMATDESCRIPTOR),
			1,
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
			PFD_TYPE_RGBA,
			32,
			0, 0, 0, 0, 0, 0,
			0, 0,
			0,
			0, 0, 0, 0,
			24, 8,
			0,
			PFD_MAIN_PLANE,
			0,
			0, 0, 0
		};
		data->h_device = GetDC(native_window->window);
		const auto pfc = ChoosePixelFormat(data->h_device, &pfd);
		if (pfc == 0) {
			log.error("Required context format not supported");
			return;
		}
		SetPixelFormat(data->h_device, pfc, &pfd);
		const auto temp_context = wglCreateContext(data->h_device);
		wglMakeCurrent(data->h_device, temp_context);

		const auto wgl_create_context_attribs_arb = reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(
			wglGetProcAddress("wglCreateContextAttribsARB")
		);

		if (wgl_create_context_attribs_arb == nullptr) {
			log.error("Cannot find context creation function");
			return;
		}

		wglMakeCurrent(data->h_device, nullptr);
		wglDeleteContext(temp_context);

		const int context_attribs[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB, ver_major,
			WGL_CONTEXT_MINOR_VERSION_ARB, ver_minor,
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			0
		};

		data->context = wgl_create_context_attribs_arb(data->h_device, nullptr, context_attribs);

		if (data->context == nullptr) {
			log.error("Failed to create context");
			return;
		}

		log.log("Successfully created OpenGL context");

		make_current();
	}

	void OpenGLPlatform::make_current() {
		if (!wglMakeCurrent(data->h_device, data->context))
			log.error("Cannot make context current");
	}

    void OpenGLPlatform::make_null_current() {
	    if (!wglMakeCurrent(nullptr, nullptr))
			log.error("Cannot make context current");
	}

    void OpenGLPlatform::swap_buffers() const {
		SwapBuffers(data->h_device);
	}

	bool OpenGLPlatform::is_init() const {
		return data != nullptr;
	}

	OpenGLPlatform::~OpenGLPlatform() {
		wglMakeCurrent(data->h_device, nullptr);
		wglDeleteContext(data->context);
		delete data;
	}
}
