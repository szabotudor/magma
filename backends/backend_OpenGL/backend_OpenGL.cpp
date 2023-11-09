#include "glad/glad.h"
#include "backend_OpenGL.hpp"

#include "logging.hpp"
#include "mgmath.hpp"

#if !defined(WIN32)
#define EXPORT extern "C"
#else
#define EXPORT extern "C" __declspec(dllexport)
#endif


namespace mgm {
    struct BackendData {
        bool init = false;
        OpenGLPlatform* platform;
        struct Viewport {
            vec2i32 pos{}, size{};
        } viewport{};
        vec4f clear_color{};
        GLbitfield clear_mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
    };

    struct Shader {
        GLuint prog = 0;
        GLuint vert = 0, frag = 0;
    };

    struct Mesh {
        GLuint vbo = 0, ebo = 0, vao = 0;
        mat4f transform{};
        Shader* shader = nullptr;
    };
    Logging log{"backend_OpenGL"};



    //==================================
    // Backend "big" graphics functions
    //==================================

    EXPORT void viewport(BackendData* data, const vec2i32& pos, const vec2i32& size) {
        data->viewport.pos = pos;
        data->viewport.size = size;
        glViewport(pos.x(), pos.y(), size.x(), size.y());
    }

    EXPORT void clear_color(BackendData* data, const vec4f& color) {
        data->clear_color = color;
        glClearColor(color.x(), color.y(), color.z(), color.w());
    }

    EXPORT void clear(BackendData* data) {
        glClear(data->clear_mask);
    }

    EXPORT void swap_buffers(BackendData* data) {
        data->platform->swap_buffers();
    }



    //============================
    // Backend graphics functions
    //============================

    [[nodiscard]] int compile_shader(GLuint sh, GLuint prog) {
        int shader_success = 0;
        char shader_log[512]{};

        glCompileShader(sh);
        glGetShaderiv(sh, GL_COMPILE_STATUS, &shader_success);
        if (!shader_success) {
            glGetShaderInfoLog(sh, 512, NULL, shader_log);
            log.error("Failed to build shader source");
            log.error(shader_log);
            return 1;
        }

        glAttachShader(prog, sh);

        return 0;
    }
    [[nodiscard]] int link_program(GLuint prog) {
        int shader_success = 0;
        char shader_log[512]{};

        glLinkProgram(prog);

        glGetProgramiv(prog, GL_LINK_STATUS, &shader_success);
        if(!shader_success) {
            glGetProgramInfoLog(prog, 512, NULL, shader_log);
            log.error("Failed to link shader program");
            log.error(shader_log);
            return 1;
        }

        return 0;
    }
    EXPORT Shader* make_shader(const void* vert_data, const void* frag_data, bool precompiled) {
        if (precompiled) {
            log.error("Precompiled shaders not supported yet");
            return nullptr;
        }
        Shader* shader = new Shader();

        shader->prog = glCreateProgram();
        shader->vert = glCreateShader(GL_VERTEX_SHADER);
        shader->frag = glCreateShader(GL_FRAGMENT_SHADER);

        glShaderSource(shader->vert, 1, (const char**)&vert_data, NULL);
        glShaderSource(shader->frag, 1, (const char**)&frag_data, NULL);

        int error_state = 0;
        error_state += compile_shader(shader->vert, shader->prog);
        error_state += compile_shader(shader->frag, shader->prog);
        error_state += link_program(shader->prog);

        if (error_state) {
            log.error("Failed to create shader");
            return nullptr;
        }

        glDeleteShader(shader->vert);
        glDeleteShader(shader->frag);
        shader->vert = 0;
        shader->frag = 0;

        log.log("Created shader", std::to_string(shader->prog).c_str());
        return shader;
    }

    EXPORT void destroy_shader(Shader* shader) {
        glDeleteProgram(shader->prog);
        log.log("Destroyed shader", std::to_string(shader->prog).c_str());
        shader->prog = 0;
    }

    EXPORT Mesh* make_mesh(Shader* shader) {
        Mesh* mesh = new Mesh();

        glGenBuffers(2, &mesh->vbo);

        return mesh;
    }

    EXPORT void mesh_data(Mesh* mesh, void* data, uint64_t size) {
        glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
        glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }



    //====================================
    // Backend initialization and freeing
    //====================================

    EXPORT void init_backend(BackendData* data, struct NativeWindow* native_window) {
        data->platform = new OpenGLPlatform{false, native_window};
        data->platform->create_context(4, 6);
        data->platform->make_current();

        gladLoadGLLoader((GLADloadproc)OpenGLPlatform::proc_address_getter);

        log.log("Initialized OpenGL Backend");
        data->init = true;
    }

    EXPORT void destroy_backend(BackendData* data) {
        log.log("Destroyed OpenGL Backend");
        data->init = false;
    }

    EXPORT BackendData* alloc_backend_data() {
        BackendData* data = new BackendData{};
        return data;
    }

    EXPORT void free_backend_data(BackendData*& data) {
        if (data->init)
            destroy_backend(data);
        delete[] data;
        data = nullptr;
    }
}
