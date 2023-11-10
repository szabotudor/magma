#include "backend_OpenGL.hpp"
#include "native_window.hpp"

#include "EGL/egl.h"


namespace mgm {
    struct OpenGLPlatform::BackendData {
        bool is_opengl_es = false;
        bool init = false;
        EGLint* attrib = nullptr;
        EGLint* context_attrib = nullptr;
        EGLDisplay display{};
        EGLConfig config{};
        EGLint num_configs{};
        EGLSurface surface{};
        EGLContext context{};
    };

    void* OpenGLPlatform::proc_address_getter(const char* name) {
        return (void*)eglGetProcAddress(name);
    }


    OpenGLPlatform::OpenGLPlatform(bool is_opengl_es) {
        data = new BackendData{};
        data->is_opengl_es = is_opengl_es;
    }

    void OpenGLPlatform::create_context(int ver_major, int ver_minor, struct NativeWindow* native_window) {
        data->display = eglGetDisplay(native_window->display);
        if (!eglInitialize(data->display, nullptr, nullptr)) {
            log.error("Failed to initialize EGL");
            return;
        }

        if (data->is_opengl_es)
            eglBindAPI(EGL_OPENGL_ES_API);
        else
            eglBindAPI(EGL_OPENGL_API);

        data->attrib = new EGLint[] {
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_DEPTH_SIZE, 24,
            EGL_STENCIL_SIZE, 8,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
            EGL_NONE
        };

        if (!eglChooseConfig(data->display, data->attrib, &data->config, 1, &data->num_configs)) {
            log.error("Failed to find suitable EGL/OpenGL configuration");
            return;
        }

        if (data->num_configs != 1) {
            log.error("Failed to find a compatible EGL configuration");
            return;
        }

        data->surface = eglCreateWindowSurface(data->display, data->config, native_window->window, nullptr);
        if (!data->surface) {
            log.error("Failed to create window surface");
            return;
        }

        data->init = true;

        data->context_attrib = new EGLint[] {
            EGL_CONTEXT_MAJOR_VERSION, ver_major,
            EGL_CONTEXT_MINOR_VERSION, ver_minor,
            EGL_NONE
        };
        data->context = eglCreateContext(data->display, data->config, EGL_NO_CONTEXT, data->context_attrib);
        if (!data->context) {
            log.error("Failed to create OpenGL context");
            return;
        }
    }

    void OpenGLPlatform::make_current() {
        if (!eglMakeCurrent(data->display, data->surface, data->surface, data->context)) {
            log.error("Failed to make context current");
            return;
        }
    }

    void OpenGLPlatform::swap_buffers() const {
        eglSwapBuffers(data->display, data->surface);
    }

    bool OpenGLPlatform::is_init() const {
        return data->init;
    }

    OpenGLPlatform::~OpenGLPlatform() {
        eglDestroySurface(data->display, data->surface);
        eglDestroyContext(data->display, data->context);
        eglTerminate(data->display);

        delete[] data->attrib;
        data->attrib = nullptr;
    }
}
