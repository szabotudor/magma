#include "engine.hpp"
#include "file.hpp"
#include "mgmlib.hpp"
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
    }

    void MagmaEngineMainLoop::tick(float delta) {
        window->update();
    }

    void MagmaEngineMainLoop::draw(float delta) {
        graphics->clear();
        graphics->swap_buffers();
    }

    bool MagmaEngineMainLoop::running() {
        return window->is_open();
    }

    MagmaEngineMainLoop::~MagmaEngineMainLoop() {
        graphics->disconnect_window();
        delete graphics;
        delete window;
    }
}


int main(int argc, char** args) {
    mgm::MagmaEngineMainLoop magma{};
    
    bool run = true;
    while (run) {
        magma.tick(1.0f);
        magma.draw(1.0f);
        run = magma.running();
    }

    return 0;
}
