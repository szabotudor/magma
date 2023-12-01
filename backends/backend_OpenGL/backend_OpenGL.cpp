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
        GLuint current_shader = 0;
    };

    struct Shader {
        BackendData* base_data = nullptr;
        GLuint prog = 0;
    };

    struct Mesh {
        GLuint verts = static_cast<GLuint>(-1), normals = static_cast<GLuint>(-1), colors = static_cast<GLuint>(-1), tex_coords = static_cast<GLuint>(-1),
            ebo = static_cast<GLuint>(-1), vao = static_cast<GLuint>(-1);
        uint32_t num_verts{};
        uint32_t num_indices{};
    };

    struct Texture {
        GLuint texture = 0;
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

    EXPORT void get_viewport(BackendData* data, vec<2, vec2i32>& ret) {
        ret = {data->viewport.pos, data->viewport.size};
    }

    EXPORT void scissor(BackendData* data, const vec2i32& pos, const vec2i32& size) {
        glScissor(pos.x(), pos.y(), size.x(), size.y());
    }

    EXPORT void clear_color(BackendData* data, const vec4f& color) {
        data->clear_color = color;
        glClearColor(color.x(), color.y(), color.z(), color.w());
    }

    EXPORT void clear(BackendData* data) {
        glClear(data->clear_mask);
    }

    EXPORT void enable_blending(BackendData* data) { glEnable(GL_BLEND); }
    EXPORT void disable_blending(BackendData* data) { glDisable(GL_BLEND); }
    EXPORT bool is_blending_enabled(BackendData* data) { return glIsEnabled(GL_BLEND); }

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
    EXPORT Shader* make_shader(BackendData* data, const void* vert_data, const void* frag_data, bool precompiled) {
        if (precompiled) {
            log.error("Precompiled shaders not supported in OpenGL");
            return nullptr;
        }
        Shader* shader = new Shader{};

        shader->prog = glCreateProgram();
        GLuint vert = glCreateShader(GL_VERTEX_SHADER);
        GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);

        glShaderSource(vert, 1, (const char**)&vert_data, NULL);
        glShaderSource(frag, 1, (const char**)&frag_data, NULL);

        int error_state = 0;
        error_state += compile_shader(vert, shader->prog);
        error_state += compile_shader(frag, shader->prog);
        error_state += link_program(shader->prog);

        if (error_state) {
            log.error("Failed to create shader");
            return nullptr;
        }

        glDeleteShader(vert);
        glDeleteShader(frag);

        shader->base_data = data;

        return shader;
    }

    EXPORT uint32_t find_uniform(const Shader* shader, const char* name) {
        const auto res = glGetUniformLocation(shader->prog, name);
        if (res == -1)
            log.error("No uniform with this name");
        return (uint32_t)res;
    }

    EXPORT void uniform_f(const Shader* shader, const uint32_t uniform_id, const float& f) {
        if (shader->base_data->current_shader != shader->prog)
            glUseProgram(shader->prog);
        glUniform1f(uniform_id, f);
    }
    EXPORT void uniform_d(const Shader* shader, const uint32_t uniform_id, const double& d) {
        if (shader->base_data->current_shader != shader->prog)
            glUseProgram(shader->prog);
        glUniform1d(uniform_id, d);
    }
    EXPORT void uniform_u(const Shader* shader, const uint32_t uniform_id, const uint32_t& u) {
        if (shader->base_data->current_shader != shader->prog)
            glUseProgram(shader->prog);
        glUniform1ui(uniform_id, u);
    }
    EXPORT void uniform_i(const Shader* shader, const uint32_t uniform_id, const int32_t& i) {
        if (shader->base_data->current_shader != shader->prog)
            glUseProgram(shader->prog);
        glUniform1i(uniform_id, i);
    }
    EXPORT void uniform_vec2f(const Shader* shader, const uint32_t uniform_id, const vec2f& v) {
        if (shader->base_data->current_shader != shader->prog)
            glUseProgram(shader->prog);
        glUniform2f(uniform_id, v.x(), v.y());
    }
    EXPORT void uniform_vec3f(const Shader* shader, const uint32_t uniform_id, const vec3f& v) {
        if (shader->base_data->current_shader != shader->prog)
            glUseProgram(shader->prog);
        glUniform3f(uniform_id, v.x(), v.y(), v.z());
    }
    EXPORT void uniform_vec4f(const Shader* shader, const uint32_t uniform_id, const vec4f& v) {
        if (shader->base_data->current_shader != shader->prog)
            glUseProgram(shader->prog);
        glUniform4f(uniform_id, v.x(), v.y(), v.z(), v.w());
    }
    EXPORT void uniform_vec2d(const Shader* shader, const uint32_t uniform_id, const vec2d& v) {
        if (shader->base_data->current_shader != shader->prog)
            glUseProgram(shader->prog);
        glUniform2d(uniform_id, v.x(), v.y());
    }
    EXPORT void uniform_vec3d(const Shader* shader, const uint32_t uniform_id, const vec3d& v) {
        if (shader->base_data->current_shader != shader->prog)
            glUseProgram(shader->prog);
        glUniform3d(uniform_id, v.x(), v.y(), v.z());
    }
    EXPORT void uniform_vec4d(const Shader* shader, const uint32_t uniform_id, const vec4d& v) {
        if (shader->base_data->current_shader != shader->prog)
            glUseProgram(shader->prog);
        glUniform4d(uniform_id, v.x(), v.y(), v.z(), v.w());
    }
    EXPORT void uniform_vec2u(const Shader* shader, const uint32_t uniform_id, const vec2u32& v) {
        if (shader->base_data->current_shader != shader->prog)
            glUseProgram(shader->prog);
        glUniform2ui(uniform_id, v.x(), v.y());
    }
    EXPORT void uniform_vec3u(const Shader* shader, const uint32_t uniform_id, const vec3u32& v) {
        if (shader->base_data->current_shader != shader->prog)
            glUseProgram(shader->prog);
        glUniform3ui(uniform_id, v.x(), v.y(), v.z());
    }
    EXPORT void uniform_vec4u(const Shader* shader, const uint32_t uniform_id, const vec4u32& v) {
        if (shader->base_data->current_shader != shader->prog)
            glUseProgram(shader->prog);
        glUniform4ui(uniform_id, v.x(), v.y(), v.z(), v.w());
    }
    EXPORT void uniform_vec2i(const Shader* shader, const uint32_t uniform_id, const vec2i32& v) {
        if (shader->base_data->current_shader != shader->prog)
            glUseProgram(shader->prog);
        glUniform2i(uniform_id, v.x(), v.y());
    }
    EXPORT void uniform_vec3i(const Shader* shader, const uint32_t uniform_id, const vec3i32& v) {
        if (shader->base_data->current_shader != shader->prog)
            glUseProgram(shader->prog);
        glUniform3i(uniform_id, v.x(), v.y(), v.z());
    }
    EXPORT void uniform_vec4i(const Shader* shader, const uint32_t uniform_id, const vec4i32& v) {
        if (shader->base_data->current_shader != shader->prog)
            glUseProgram(shader->prog);
        glUniform4i(uniform_id, v.x(), v.y(), v.z(), v.w());
    }
    EXPORT void uniform_mat4f(const Shader* shader, const uint32_t uniform_id, const mat4f& m) {
        if (shader->base_data->current_shader != shader->prog)
            glUseProgram(shader->prog);
        glUniformMatrix4fv(uniform_id, 1, false, (const float*)&m);
    }

    EXPORT void destroy_shader(Shader* shader) {
        glDeleteProgram(shader->prog);
        delete shader;
    }

    template<typename T>
    void make_buffer(const T* data, const uint32_t num_per_point, const GLenum gl_data_type, const bool normalized, uint32_t data_len,
                    GLuint& buff, const GLenum gl_buffer_type, uint32_t attrib_array_id, GLenum draw_mode) {
        glGenBuffers(1, &buff);
        glBindBuffer(gl_buffer_type, buff);
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

        mesh->num_verts = num_verts;
        mesh->num_indices = num_indices;

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        return mesh;
    }

    EXPORT void change_mesh_data(Mesh* mesh, const vec3f* verts, const vec3f* normals, const vec4f* colors, const vec2f* tex_coords, const uint32_t num_verts) {
        if (mesh->verts != (GLuint)-1) glDeleteBuffers(1, &mesh->verts);
        if (mesh->normals != (GLuint)-1) glDeleteBuffers(1, &mesh->normals);
        if (mesh->colors != (GLuint)-1) glDeleteBuffers(1, &mesh->colors);
        if (mesh->tex_coords != (GLuint)-1) glDeleteBuffers(1, &mesh->tex_coords);

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

        mesh->num_verts = num_verts;

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    EXPORT void change_mesh_indices(Mesh* mesh, const uint32_t* indices, const uint32_t num_indices) {
        if (mesh->ebo != (GLuint)-1) glDeleteBuffers(1, &mesh->ebo);

        glBindVertexArray(mesh->vao);

        if (indices) make_buffer(
            indices, 1, GL_UNSIGNED_INT, false, num_indices,
            mesh->ebo, GL_ELEMENT_ARRAY_BUFFER, (uint32_t)-1, GL_STATIC_DRAW
        );

        mesh->num_indices = num_indices;

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    
    EXPORT void destroy_mesh(Mesh* mesh) {
        glDeleteVertexArrays(1, &mesh->vao);

        if (mesh->verts != (GLuint)-1) glDeleteBuffers(1, &mesh->verts);
        if (mesh->normals != (GLuint)-1) glDeleteBuffers(1, &mesh->normals);
        if (mesh->colors != (GLuint)-1) glDeleteBuffers(1, &mesh->colors);
        if (mesh->tex_coords != (GLuint)-1) glDeleteBuffers(1, &mesh->tex_coords);

        if (mesh->ebo != (GLuint)-1) glDeleteBuffers(1, &mesh->ebo);

        delete mesh;
    }

    EXPORT Texture* make_texture(const uint8_t* data, const vec2u32 size, const bool use_alpha, const bool use_mipmaps) {
        Texture* res = new Texture{};
        glCreateTextures(GL_TEXTURE_2D, 1, &res->texture);
        glBindTexture(GL_TEXTURE_2D, res->texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        if (use_mipmaps)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        else
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        if (use_alpha)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x(), size.y(), 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        else
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size.x(), size.y(), 0, GL_RGB, GL_UNSIGNED_BYTE, data);

        if (use_mipmaps)
            glGenerateMipmap(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, 0);
        return res;
    }

    EXPORT void destroy_texture(Texture* texture) {
        glDeleteTextures(1, &texture->texture);
        delete texture;
    }

    EXPORT void draw(const Mesh* mesh, const Shader* shader, const Texture* const* textures, const uint32_t num_textures) {
        if (shader->base_data->current_shader != shader->prog)
            glUseProgram(shader->prog);

        for (uint32_t i = 0; i < num_textures; i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, textures[i]->texture);
        }

        glBindVertexArray(mesh->vao);
        if (mesh->num_indices)
            glDrawElements(GL_TRIANGLES, mesh->num_indices, GL_UNSIGNED_INT, 0);
        else
            glDrawArrays(GL_TRIANGLES, 0, mesh->num_verts);

        glBindVertexArray(0);
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

        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
        glEnable(GL_SCISSOR_TEST);
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
