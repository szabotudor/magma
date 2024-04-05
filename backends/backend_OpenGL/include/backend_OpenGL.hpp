#pragma once
#include "logging.hpp"
#include <cstdint>

#if defined(__linux__) || defined(WIN32) || defined(_WIN32)
#define PLATFORM_DESKTOP
#endif


namespace mgm {
    class OpenGLPlatform {
        struct EGLBackendData;
        Logging log{"OpenGL Platform"};
        EGLBackendData* data = nullptr;

        public:

        static void* proc_address_getter(const char*);

        OpenGLPlatform(const OpenGLPlatform&) = delete;
        OpenGLPlatform(OpenGLPlatform&&) = delete;
        OpenGLPlatform& operator=(const OpenGLPlatform&) = delete;
        OpenGLPlatform& operator=(OpenGLPlatform&&) = delete;

        OpenGLPlatform(bool is_opengl_es);

        void create_context(int ver_major, int ver_minor, struct NativeWindow* native_window);

        void make_current();

        void swap_buffers() const;

        bool is_init() const;

        ~OpenGLPlatform();
    };
}
