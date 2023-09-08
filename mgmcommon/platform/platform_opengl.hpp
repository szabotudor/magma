#pragma once
#include "logging.hpp"
#include "mgmath.hpp"


namespace mgm {
    class OpenGLPlatform {
        struct BackendData;
        logging log{"OpenGL Platform"};
        BackendData* data = nullptr;

        public:

        static void* proc_address_getter;

        OpenGLPlatform(const OpenGLPlatform&) = delete;
        OpenGLPlatform(OpenGLPlatform&&) = delete;
        OpenGLPlatform& operator=(const OpenGLPlatform&) = delete;
        OpenGLPlatform& operator=(OpenGLPlatform&&) = delete;

        OpenGLPlatform(bool is_opengl_es, void* native_display, uint32_t native_window);

        void create_context(int ver_major, int ver_minor);

        void make_current();

        void swap_buffers();

        bool is_init();

        ~OpenGLPlatform();
    };
}
