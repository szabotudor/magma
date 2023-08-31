#include "engine.hpp"
#include "file.hpp"
#include "mgmlib.hpp"


namespace mgm {
    MagmaEngineMainLoop::MagmaEngineMainLoop() {
        graphics.load_backend((MgmFile::exe_dir() + "/shared/libbackend_OpenGL.so").c_str());
        graphics.connect_to_window(window);
        graphics.viewport(vec2i32(0, 0), vec2i32(800, 600));
        graphics.clear_color(vec4f(0.2f, 0.3f, 0.4f, 1.0f));
        MgmGraphics::Shader sh = graphics.make_shader_from_source(
            "#version 460 core\nvoid main() { }",
            "#version 460 core\nvoid main() { }"
        );
    }

    #ifndef NDEBUG
    int MagmaEngineMainLoop::tests() {
        return 0;
    }
    #endif

    void MagmaEngineMainLoop::init() {
    }

    bool MagmaEngineMainLoop::tick(float delta) {
        window.update();
        graphics.clear();
        graphics.swap_buffers();
        return window.is_open();
    }

    void MagmaEngineMainLoop::draw(float delta) {
    }

    MagmaEngineMainLoop::~MagmaEngineMainLoop() {
        graphics.disconnect_window();
    }
}


int main(int argc, char** args) {
    mgm::MagmaEngineMainLoop magma{};
    magma.init();

    #ifndef NDEBUG
    int test_res = magma.tests();
    if (test_res)
        return test_res;
    #endif
    
    bool run = true;
    while (run) {
        run = magma.tick(1.0f);
        magma.draw(1.0f);
    }

    return 0;
}
