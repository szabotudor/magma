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
        lib->sym("make_mesh", &funcs.make_mesh);
        lib->sym("destroy_mesh", &funcs.destroy_mesh);
        lib->sym("draw", &funcs.draw);

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

    MgmGraphics::~MgmGraphics() {
        disconnect_window();
        unload_backend();
        delete log;
    }


    MgmGraphics::ShaderHandle MgmGraphics::make_shader_from_source(const char* vert, const char* frag) {
        const auto sp = funcs.make_shader(vert, frag, false);
        if (sp == nullptr) {
            log->error("Failed to create shader");
            return bad_shader;
        }
        if (!deleted_shaders.empty()) {
            const auto s = deleted_shaders.back();
            shaders[s] = sp;
            deleted_shaders.pop_back();
            return s;
        }
        shaders.emplace_back(sp);
        return (ShaderHandle)(shaders.size() - 1);
    }

    void MgmGraphics::destroy_shader(const MgmGraphics::ShaderHandle shader) {
        if (shaders[shader] == nullptr) {
            log->error("Cannot destroy shader, it doesn't exist");
            return;
        }

        funcs.destroy_shader(shaders[shader]);
        shaders[shader] = nullptr;
        deleted_shaders.emplace_back(shader);
    }

    MgmGraphics::MeshHandle MgmGraphics::make_mesh(const vec3f* verts, const vec3f* normals, const vec4f* colors, const vec2f* tex_coords, const uint32_t num_verts,
                            const uint32_t* indices, const uint32_t num_indices) {
        const auto mp = funcs.make_mesh(verts, normals, colors, tex_coords, num_verts, indices, num_indices);
        if (mp == nullptr) {
            log->error("Failed to create mesh");
            return bad_mesh;
        }
        if (!deleted_meshes.empty()) {
            const auto m = deleted_meshes.back();
            meshes[m] = mp;
            deleted_meshes.pop_back();
            return m;
        }
        meshes.emplace_back(mp);
        return (MeshHandle)(meshes.size() - 1);
    }

    void MgmGraphics::destroy_mesh(const MeshHandle mesh) {
        if (meshes[mesh] == nullptr) {
            log->error("Cannot destroy mesh, it doesn't exist");
            return;
        }

        funcs.destroy_mesh(meshes[mesh]);
        meshes[mesh] = nullptr;
        deleted_meshes.emplace_back(mesh);
    }
}
