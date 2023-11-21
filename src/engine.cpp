#include "engine.hpp"
#include "file.hpp"
#include "mgmgfx.hpp"
#include "mgmwin.hpp"


namespace mgm {
    MagmaEngineMainLoop::MagmaEngineMainLoop() {
        window = new MgmWindow{"Hello", {}, vec2u32{800, 600}, MgmWindow::Mode::NORMAL};
        graphics = new MgmGraphics{};
#if defined(__linux__)
        graphics->load_backend("shared/libbackend_OpenGL.so");
#elif defined(WIN32)
        graphics->load_backend("shared/backend_OpenGL.dll");
#endif
        graphics->connect_to_window(*window);
        graphics->clear_color(vec4f{0.1f, 0.2f, 0.3f, 1.0f});

        static const vec3f verts[] = {vec3f{-0.5f, -0.5f, 0.0f}, vec3f{0.0f, 0.5f, 0.0f}, vec3f{0.5f, -0.5f, 0.0f}};
        const auto mesh = graphics->make_mesh(
            verts,
            {}, {}, {}, 3,
            nullptr, 0
        );

        graphics->make_shader_from_source(
            "#version 460 core\n layout (location = 0) in vec3 a_pos; void main() { gl_Position = vec4(a_pos, 1.0f); }",
            "#version 460 core\n out vec4 FragColor; void main() { FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f); }"
        );
    }

    void MagmaEngineMainLoop::tick(float delta) {
        window->update();
    }

    void MagmaEngineMainLoop::draw() {
        graphics->clear();
        graphics->draw(0, 0);
        graphics->swap_buffers();
    }

    bool MagmaEngineMainLoop::running() {
        return window->is_open();
    }

    MagmaEngineMainLoop::~MagmaEngineMainLoop() {
        graphics->destroy_shader(0);
        graphics->destroy_mesh(0);
        graphics->disconnect_from_window();
        delete graphics;
        delete window;
    }
}


int main(int argc, char** args) {
    mgm::MagmaEngineMainLoop magma{};
    
    bool run = true;
    while (run) {
        magma.tick(1.0f);
        magma.draw();
        run = magma.running();
    }

    return 0;
}
