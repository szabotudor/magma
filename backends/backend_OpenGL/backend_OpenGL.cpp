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
        GLuint verts = -1, normals = -1, colors = -1, tex_coords = -1,
            ebo = -1, vao = -1;
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
            log.error("Precompiled shaders not supported in OpenGL");
            return nullptr;
        }
        Shader* shader = new Shader{};

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

        return shader;
    }

    EXPORT uint32_t find_uniform(const Shader* shader, const char* name) {
        const auto res = glGetUniformLocation(shader->prog, name);
        if (res == -1)
            log.error("No uniform with this name");
        return (uint32_t)res;
    }

    EXPORT void uniform_f(const Shader* shader, const uint32_t uniform_id, const float& f) {
        glUseProgram(shader->prog);
        glUniform1f(uniform_id, f);
        glUseProgram(0);
    }
    EXPORT void uniform_d(const Shader* shader, const uint32_t uniform_id, const double& d) {
        glUseProgram(shader->prog);
        glUniform1d(uniform_id, d);
        glUseProgram(0);
    }
    EXPORT void uniform_u(const Shader* shader, const uint32_t uniform_id, const uint32_t& u) {
        glUseProgram(shader->prog);
        glUniform1ui(uniform_id, u);
        glUseProgram(0);
    }
    EXPORT void uniform_i(const Shader* shader, const uint32_t uniform_id, const int32_t& i) {
        glUseProgram(shader->prog);
        glUniform1i(uniform_id, i);
        glUseProgram(0);
    }
    EXPORT void uniform_vec2f(const Shader* shader, const uint32_t uniform_id, const vec2f& v) {
        glUseProgram(shader->prog);
        glUniform2f(uniform_id, v.x(), v.y());
        glUseProgram(0);
    }
    EXPORT void uniform_vec3f(const Shader* shader, const uint32_t uniform_id, const vec3f& v) {
        glUseProgram(shader->prog);
        glUniform3f(uniform_id, v.x(), v.y(), v.z());
        glUseProgram(0);
    }
    EXPORT void uniform_vec4f(const Shader* shader, const uint32_t uniform_id, const vec4f& v) {
        glUseProgram(shader->prog);
        glUniform4f(uniform_id, v.x(), v.y(), v.z(), v.w());
        glUseProgram(0);
    }
    EXPORT void uniform_vec2d(const Shader* shader, const uint32_t uniform_id, const vec2d& v) {
        glUseProgram(shader->prog);
        glUniform2d(uniform_id, v.x(), v.y());
        glUseProgram(0);
    }
    EXPORT void uniform_vec3d(const Shader* shader, const uint32_t uniform_id, const vec3d& v) {
        glUseProgram(shader->prog);
        glUniform3d(uniform_id, v.x(), v.y(), v.z());
        glUseProgram(0);
    }
    EXPORT void uniform_vec4d(const Shader* shader, const uint32_t uniform_id, const vec4d& v) {
        glUseProgram(shader->prog);
        glUniform4d(uniform_id, v.x(), v.y(), v.z(), v.w());
        glUseProgram(0);
    }
    EXPORT void uniform_vec2u(const Shader* shader, const uint32_t uniform_id, const vec2u32& v) {
        glUseProgram(shader->prog);
        glUniform2ui(uniform_id, v.x(), v.y());
        glUseProgram(0);
    }
    EXPORT void uniform_vec3u(const Shader* shader, const uint32_t uniform_id, const vec3u32& v) {
        glUseProgram(shader->prog);
        glUniform3ui(uniform_id, v.x(), v.y(), v.z());
        glUseProgram(0);
    }
    EXPORT void uniform_vec4u(const Shader* shader, const uint32_t uniform_id, const vec4u32& v) {
        glUseProgram(shader->prog);
        glUniform4ui(uniform_id, v.x(), v.y(), v.z(), v.w());
        glUseProgram(0);
    }
    EXPORT void uniform_vec2i(const Shader* shader, const uint32_t uniform_id, const vec2i32& v) {
        glUseProgram(shader->prog);
        glUniform2i(uniform_id, v.x(), v.y());
        glUseProgram(0);
    }
    EXPORT void uniform_vec3i(const Shader* shader, const uint32_t uniform_id, const vec3i32& v) {
        glUseProgram(shader->prog);
        glUniform3i(uniform_id, v.x(), v.y(), v.z());
        glUseProgram(0);
    }
    EXPORT void uniform_vec4i(const Shader* shader, const uint32_t uniform_id, const vec4i32& v) {
        glUseProgram(shader->prog);
        glUniform4i(uniform_id, v.x(), v.y(), v.z(), v.w());
        glUseProgram(0);
    }

    EXPORT void destroy_shader(Shader* shader) {
        glDeleteProgram(shader->prog);
        delete shader;
    }

    template<typename T>
    void make_buffer(const T* data, const uint32_t num_per_point, const GLenum gl_data_type, const bool normalized, uint32_t data_len,
                    GLuint& vbo, const GLenum gl_buffer_type, uint32_t attrib_array_id, GLenum draw_mode) {
        glGenBuffers(1, &vbo);
        glBindBuffer(gl_buffer_type, vbo);
        glBufferData(gl_buffer_type, data_len * sizeof(T), data, draw_mode);
        if (attrib_array_id != (uint32_t)-1) {
            glVertexAttribPointer(attrib_array_id, num_per_point, gl_data_type, normalized, sizeof(T), nullptr);
            glEnableVertexAttribArray(attrib_array_id);
        }
    }

    EXPORT Mesh* make_mesh(const vec3f* verts, const vec3f* normals, const vec4f* colors, const vec2f* tex_coords, const uint32_t num_verts,
                            const uint32_t* indices, const uint32_t num_indices) {
        if (!verts) {
            log.error("Mesh must contain vertices");
            return nullptr;
        }

        Mesh* mesh = new Mesh{};

        glGenVertexArrays(1, &mesh->vao);
        glBindVertexArray(mesh->vao);

        make_buffer(
            verts, 3, GL_FLOAT, false, num_verts, 
            mesh->verts, GL_ARRAY_BUFFER, 0, GL_STATIC_DRAW
        );

        if (normals) make_buffer(
            normals, 3, GL_FLOAT, true, num_verts, 
            mesh->normals, GL_ARRAY_BUFFER, 1, GL_STATIC_DRAW
        );

        if (colors) make_buffer(
            colors, 4, GL_FLOAT, false, num_verts, 
            mesh->colors, GL_ARRAY_BUFFER, 2, GL_STATIC_DRAW
        );

        if (tex_coords) make_buffer(
            tex_coords, 2, GL_FLOAT, false, num_verts, 
            mesh->tex_coords, GL_ARRAY_BUFFER, 3, GL_STATIC_DRAW
        );

        if (indices) make_buffer(
            indices, 1, GL_UNSIGNED_INT, false, num_indices,
            mesh->ebo, GL_ELEMENT_ARRAY_BUFFER, (uint32_t)-1, GL_STATIC_DRAW
        );

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        return mesh;
    }
    
    EXPORT void destroy_mesh(Mesh* mesh) {
        glDeleteVertexArrays(1, &mesh->vao);

        if (mesh->verts != (GLuint)-1) glDeleteBuffers(1, &mesh->verts);
        if (mesh->normals != (GLuint)-1) glDeleteBuffers(1, &mesh->normals);
        if (mesh->colors != (GLuint)-1) glDeleteBuffers(1, &mesh->colors);
        if (mesh->tex_coords != (GLuint)-1) glDeleteBuffers(1, &mesh->tex_coords);

        glDeleteBuffers(1, &mesh->ebo);

        delete mesh;
    }

    EXPORT void draw(const Mesh* mesh, const Shader* shader) {
        glBindVertexArray(mesh->vao);
        glUseProgram(shader->prog);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glBindVertexArray(0);
        glUseProgram(0);
    }



    //====================================
    // Backend initialization and freeing
    //====================================

    EXPORT void init_backend(BackendData* data, struct NativeWindow* native_window) {
        data->platform = new OpenGLPlatform{false};
        data->platform->create_context(4, 6, native_window);
        data->platform->make_current();

        if (!gladLoadGLLoader((GLADloadproc)OpenGLPlatform::proc_address_getter)) {
	        log.error("Could not load GLAD");
            return;
        }

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
