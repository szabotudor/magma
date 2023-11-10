#include "dloader.hpp"
#include "logging.hpp"
#include "mgmath.hpp"
#include "mgmlib.hpp"
#include "mgmwin.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>


namespace mgm {
    MgmGraphics::Shader::Shader(MgmGraphics::DShader* native_shader, MgmGraphics& graphics) {
        shader = native_shader;
        funcs.destroy_shader = graphics.funcs.destroy_shader;
    }

    MgmGraphics::Shader::Shader(MgmGraphics::Shader&& sh) {
        shader = sh.shader;
        funcs.destroy_shader = sh.funcs.destroy_shader;
        sh.shader = nullptr;
        sh.funcs.destroy_shader = nullptr;
    }

    MgmGraphics::Shader& MgmGraphics::Shader::operator=(MgmGraphics::Shader&& sh) {
        shader = sh.shader;
        funcs.destroy_shader = sh.funcs.destroy_shader;
        sh.shader = nullptr;
        sh.funcs.destroy_shader = nullptr;
        return *this;
    }
    
    MgmGraphics::DShader* MgmGraphics::Shader::get_native_shader() {
        return shader;
    }

    MgmGraphics::Shader::~Shader() {
        funcs.destroy_shader(shader);
    }
    
    
    MgmGraphics::MgmGraphics(const char* backend_path) {
        log = new Logging{"Graphics"};
        if (backend_path != nullptr)
            load_backend(backend_path);
    }

    bool MgmGraphics::is_backend_loaded() {
        return lib != nullptr;
    }

    void MgmGraphics::load_backend(const char* path) {
        if (is_backend_loaded())
            unload_backend();
        lib = new DLoader(path);
        if (!lib->is_loaded()) {
            log->error("Failed to load backend");
            return;
        }

        lib->sym("alloc_backend_data", &funcs.alloc_backend_data);
        lib->sym("free_backend_data", &funcs.free_backend_data);

        lib->sym("init_backend", &funcs.init_backend);
        lib->sym("destroy_backend", &funcs.destroy_backend);

        lib->sym("viewport", &funcs.viewport);
        lib->sym("clear_color", &funcs.clear_color);
        lib->sym("clear", &funcs.clear);
        lib->sym("swap_buffers", &funcs.swap_buffers);

        lib->sym("make_shader", &funcs.make_shader);
        lib->sym("destroy_shader", &funcs.destroy_shader);

        data = funcs.alloc_backend_data();

        log->log("Loaded backend");
    }

    void MgmGraphics::unload_backend() {
        if (!is_backend_loaded())
            return;

        funcs.free_backend_data(data);
        delete lib;
        lib = nullptr;
        memset(&funcs, 0, sizeof(funcs));

        log->log("Unloaded backend");
    }
    
    void MgmGraphics::connect_to_window(MgmWindow& window) {
        if (lib == nullptr)
            throw std::runtime_error("Tried to connect uninitialized graphics to window");
        funcs.init_backend(data, window.get_native_window());
        log->log("Connected graphics to window");
    }

    void MgmGraphics::disconnect_window() {
        if (!window_connected)
            return;
        funcs.destroy_backend(data);
        log->log("Disconnected window");
    }

    MgmGraphics::Shader MgmGraphics::make_shader_from_source(const char* vert, const char* frag) {
        return Shader{funcs.make_shader(vert, frag, false), *this};
    }

    MgmGraphics::~MgmGraphics() {
        disconnect_window();
        unload_backend();
    }
}
