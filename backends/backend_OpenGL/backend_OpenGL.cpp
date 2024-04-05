#include <algorithm>
#include <atomic>
#include <cstddef>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <any>

#include "types.hpp"

#include "glad/glad.h"
#include "backend_OpenGL.hpp"

#include "backend_settings.hpp"
#include "logging.hpp"
#include "mgmath.hpp"

#if !defined(WIN32)
#define EXPORT extern "C"
#else
#define EXPORT extern "C" __declspec(dllexport)
#endif


namespace mgm {
    struct Buffer {
        GLuint buffer = 0;
        GLint gl_data_type = 0;
        size_t gl_data_type_point_count = 0;
        size_t size = 0;
        size_t data_point_size = 0;
        bool is_element_array: 1 = false;

        ~Buffer() {
            if (buffer)
                glDeleteBuffers(1, &buffer);
        }
    };

    struct BuffersObject {
        std::vector<Buffer*> buffers{};
        GLuint vao = 0;
        bool has_index_buffer = false;

        ~BuffersObject() {
            if (vao)
                glDeleteVertexArrays(1, &vao);
        }
    };

    struct Shader {
        enum class Type {
            GRAPHICS, COMPUTE
        } type = Type::GRAPHICS;
        GLuint prog = 0;
        std::unordered_map<std::string, GLuint> uniform_locations{};

        ~Shader() {
            if (prog)
                glDeleteProgram(prog);
        }
    };

    struct Texture {
        GLuint tex = 0;
        vec2i32 size{};

        ~Texture() {
            if (tex)
                glDeleteTextures(1, &tex);
        }
    };

    struct BackendData {
        bool init = false;
        OpenGLPlatform* platform = nullptr;

        struct Viewport {
            vec2i32 pos{}, size{};
        } viewport{};
        using Scissor = Viewport;
        Scissor scissor{};

        vec4f clear_color{};
        GLbitfield clear_mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
        GLuint current_shader = 0;
        
        struct DrawCall {
            Shader* shader = nullptr;
            BuffersObject* buffers_object = nullptr;
            std::vector<Texture*> textures{};
            std::unordered_map<std::string, std::any> parameters{};
        };
        std::vector<DrawCall> draw_calls{};
    };

    thread_local Logging log{"backend_OpenGL"};
    std::atomic_bool initialized{false};



    //============================
    // Backend graphics functions
    //============================

    GLenum gl_blending_factor(Settings::Blending::Factor factor) {
        switch (factor) {
            case Settings::Blending::Factor::ZERO: return GL_ZERO;
            case Settings::Blending::Factor::ONE: return GL_ONE;
            case Settings::Blending::Factor::SRC_COLOR: return GL_SRC_COLOR;
            case Settings::Blending::Factor::ONE_MINUS_SRC_COLOR: return GL_ONE_MINUS_SRC_COLOR;
            case Settings::Blending::Factor::DST_COLOR: return GL_DST_COLOR;
            case Settings::Blending::Factor::ONE_MINUS_DST_COLOR: return GL_ONE_MINUS_DST_COLOR;
            case Settings::Blending::Factor::SRC_ALPHA: return GL_SRC_ALPHA;
            case Settings::Blending::Factor::ONE_MINUS_SRC_ALPHA: return GL_ONE_MINUS_SRC_ALPHA;
            case Settings::Blending::Factor::DST_ALPHA: return GL_DST_ALPHA;
            case Settings::Blending::Factor::ONE_MINUS_DST_ALPHA: return GL_ONE_MINUS_DST_ALPHA;
        }
        return GL_INVALID_ENUM;
    }
    GLenum gl_blending_equation(Settings::Blending::Equation equation) {
        switch (equation) {
            case Settings::Blending::Equation::ADD: return GL_FUNC_ADD;
            case Settings::Blending::Equation::SRC_MINUS_DST: return GL_FUNC_SUBTRACT;
            case Settings::Blending::Equation::DST_MINUS_SRC: return GL_FUNC_REVERSE_SUBTRACT;
            case Settings::Blending::Equation::MIN: return GL_MIN;
            case Settings::Blending::Equation::MAX: return GL_MAX;
        }
        return GL_INVALID_ENUM;
    }

    EXPORT bool set_attribute(BackendData* backend, const Settings::StateAttribute& attr, const void* data) {
        switch (attr) {
            case Settings::StateAttribute::CLEAR: {
                const auto& clear = *static_cast<const Settings::Settings::Clear*>(data);
                if (clear.enabled) {
                    backend->clear_color = clear.color;
                    backend->clear_mask = clear.color_buffer * GL_COLOR_BUFFER_BIT
                        | clear.depth_buffer * GL_DEPTH_BUFFER_BIT
                        | clear.stencil_buffer * GL_STENCIL_BUFFER_BIT;
                    glClearColor(clear.color.x(), clear.color.y(), clear.color.z(), clear.color.w());
                    glClearDepth(1.0);
                    glClearStencil(0);
                }
                else
                    backend->clear_mask = 0;
                return true;
            }
            case Settings::StateAttribute::BLENDING: {
                const auto& blending = *static_cast<const Settings::Settings::Blending*>(data);
                if (blending.enabled) {
                    glEnable(GL_BLEND);
                    glBlendFuncSeparate(
                        gl_blending_factor(blending.src_color_factor),
                        gl_blending_factor(blending.dst_color_factor),
                        gl_blending_factor(blending.src_alpha_factor),
                        gl_blending_factor(blending.dst_alpha_factor)
                    );
                    glBlendEquationSeparate(
                        gl_blending_equation(blending.color_equation),
                        gl_blending_equation(blending.alpha_equation)
                    );
                }
                else
                    glDisable(GL_BLEND);
                return true;
            }
            case Settings::StateAttribute::VIEWPORT: {
                const auto& viewport = *static_cast<const Settings::Settings::Viewport*>(data);
                backend->viewport.pos = viewport.top_left;
                backend->viewport.size = viewport.bottom_right - viewport.top_left;
                glViewport(
                    backend->viewport.pos.x(), backend->viewport.pos.y(),
                    backend->viewport.size.x(), backend->viewport.size.y()
                );
                return true;
            }
            case Settings::StateAttribute::SCISSOR: {
                const auto& scissor = *static_cast<const Settings::Settings::Scissor*>(data);
                backend->scissor.pos = scissor.top_left;
                backend->scissor.size = scissor.bottom_right - scissor.top_left;
                glScissor(
                    backend->scissor.pos.x(), backend->scissor.pos.y(),
                    backend->scissor.size.x(), backend->scissor.size.y()
                );
                return true;
            }
        }
        return false;
    }

    EXPORT void clear(BackendData* backend) {
        if (backend->clear_mask)
            glClear(backend->clear_mask);
    }

    void set_uniform(const GLuint uniform, const std::any& value) {
        if (value.type() == typeid(int)) {
            glUniform1i(uniform, std::any_cast<int>(value));
        }
        else if (value.type().hash_code() == typeid(float).hash_code()) {
            glUniform1f(uniform, std::any_cast<float>(value));
        }
        else if (value.type().hash_code() == typeid(vec2f).hash_code()) {
            const auto& vec = std::any_cast<vec2f>(value);
            glUniform2f(uniform, vec.x(), vec.y());
        }
        else if (value.type().hash_code() == typeid(vec3f).hash_code()) {
            const auto& vec = std::any_cast<vec3f>(value);
            glUniform3f(uniform, vec.x(), vec.y(), vec.z());
        }
        else if (value.type().hash_code() == typeid(vec4f).hash_code()) {
            const auto& vec = std::any_cast<vec4f>(value);
            glUniform4f(uniform, vec.x(), vec.y(), vec.z(), vec.w());
        }
        else if (value.type().hash_code() == typeid(mat2f).hash_code()) {
            const auto& mat = std::any_cast<mat2f>(value);
            glUniformMatrix2fv(uniform, 1, GL_FALSE, (float*)&mat.data);
        }
        else if (value.type().hash_code() == typeid(mat3f).hash_code()) {
            const auto& mat = std::any_cast<mat3f>(value);
            glUniformMatrix3fv(uniform, 1, GL_FALSE, (float*)&mat.data);
        }
        else if (value.type().hash_code() == typeid(mat4f).hash_code()) {
            const auto& mat = std::any_cast<mat4f>(value);
            glUniformMatrix4fv(uniform, 1, GL_FALSE, (float*)&mat.data);
        }
        else
            log.error("Unsupported shader parameter type '", value.type().name(), "'");
    }

    EXPORT void execute(BackendData* backend) {
        for (const auto& draw_call : backend->draw_calls) {
            glUseProgram(draw_call.shader->prog);
            if (!draw_call.parameters.empty()) {
                for (const auto& [name, value] : draw_call.parameters) {
                    const auto it = draw_call.shader->uniform_locations.find(name);
                    if (it == draw_call.shader->uniform_locations.end()) {
                        const auto loc = glGetUniformLocation(draw_call.shader->prog, name.c_str());
                        if (loc == -1) {
                            log.error("Could not find shader parameter '", name, "' in shader");
                            continue;
                        }
                        draw_call.shader->uniform_locations[name] = loc;
                        set_uniform(loc, value);
                    }
                    else
                        set_uniform(it->second, value);
                }
            }

            if (!draw_call.textures.empty()) {
                for (size_t i = 0; i < draw_call.textures.size(); i++) {
                    glActiveTexture(GL_TEXTURE0 + i);
                    glBindTexture(GL_TEXTURE_2D, draw_call.textures[i]->tex);
                }
            }

            size_t data_point_count = (size_t)-1;

            for (const auto& buf : draw_call.buffers_object->buffers) {
                if (buf->is_element_array) {
                    data_point_count = buf->size;
                    continue;
                }
                data_point_count = std::min(data_point_count, buf->size);
            }

            glBindVertexArray(draw_call.buffers_object->vao);
            if (draw_call.buffers_object->has_index_buffer)
                glDrawElements(GL_TRIANGLES, data_point_count, GL_UNSIGNED_INT, nullptr);
            else
                glDrawArrays(GL_TRIANGLES, 0, data_point_count);
        }
        backend->draw_calls.clear();
    }

    EXPORT void present(BackendData* backend) {
        backend->platform->swap_buffers();
    }

    constexpr auto GL_INVALID = (GLint)-1;

    auto gl_type_id(size_t raw_type_id) {
        static const std::unordered_map<size_t, GLint> type_map{
            {typeid(float).hash_code(), GL_FLOAT},
            {typeid(double).hash_code(), GL_DOUBLE},
            {typeid(int).hash_code(), GL_INT},
            {typeid(unsigned int).hash_code(), GL_UNSIGNED_INT},
            {typeid(short).hash_code(), GL_SHORT},
            {typeid(unsigned short).hash_code(), GL_UNSIGNED_SHORT},
            {typeid(char).hash_code(), GL_BYTE},
            {typeid(unsigned char).hash_code(), GL_UNSIGNED_BYTE},
            {typeid(vec2f).hash_code(), GL_FLOAT},
            {typeid(vec3f).hash_code(), GL_FLOAT},
            {typeid(vec4f).hash_code(), GL_FLOAT},
            {typeid(mat2f).hash_code(), GL_FLOAT},
            {typeid(mat3f).hash_code(), GL_FLOAT},
            {typeid(mat4f).hash_code(), GL_FLOAT}
        };
        const auto it = type_map.find(raw_type_id);
        if (it == type_map.end())
            return GL_INVALID;
        const auto gl_type = it->second;
        return gl_type;
    }

    auto gl_type_point_count(size_t raw_type_id) {
        static const std::unordered_map<size_t, size_t> type_map{
            {typeid(float).hash_code(), 1},
            {typeid(double).hash_code(), 1},
            {typeid(int).hash_code(), 1},
            {typeid(unsigned int).hash_code(), 1},
            {typeid(short).hash_code(), 1},
            {typeid(unsigned short).hash_code(), 1},
            {typeid(char).hash_code(), 1},
            {typeid(unsigned char).hash_code(), 1},
            {typeid(vec2f).hash_code(), 2},
            {typeid(vec3f).hash_code(), 3},
            {typeid(vec4f).hash_code(), 4},
            {typeid(mat2f).hash_code(), 4},
            {typeid(mat3f).hash_code(), 9},
            {typeid(mat4f).hash_code(), 16}
        };
        const auto it = type_map.find(raw_type_id);
        if (it == type_map.end())
            return (size_t)GL_INVALID;
        const auto point_count = it->second;
        return point_count;
    }

    EXPORT void buffer_data(BackendData*, Buffer* buffer, void* data, size_t size);
    EXPORT void* create_buffer(BackendData*, const BufferCreateInfo& info) {
        const auto gl_buffer_type = info.type() == BufferCreateInfo::Type::RAW ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER;
        if (gl_buffer_type == GL_INVALID)
            return nullptr;
        GLuint buf{};
        glGenBuffers(1, &buf);

        Buffer* buffer = new Buffer{
            .buffer = buf,
            .gl_data_type = gl_type_id(info.type_id_hash()),
            .gl_data_type_point_count = gl_type_point_count(info.type_id_hash()),
            .size = info.size(),
            .data_point_size = info.data_point_size(),
            .is_element_array = info.type() == BufferCreateInfo::Type::INDEX
        };
        
        buffer_data(nullptr, buffer, info.data(), info.size());
        return buffer;
    }

    EXPORT void buffer_data(BackendData*, Buffer* buffer, void* data, size_t size) {
        const auto gl_buffer_type = buffer->is_element_array ? GL_ELEMENT_ARRAY_BUFFER : GL_ARRAY_BUFFER;
        glBindBuffer(gl_buffer_type, buffer->buffer);
        glBufferData(gl_buffer_type, size * buffer->data_point_size, data, GL_STATIC_DRAW);
        glBindBuffer(gl_buffer_type, 0);
        buffer->size = size;
    }

    EXPORT void destroy_buffer(BackendData*, Buffer* buffer) {
        delete buffer;
    }

    EXPORT BuffersObject* create_buffers_object(BackendData*, Buffer** buffers, size_t count) {
        const auto buffers_object = new BuffersObject{};

        GLuint vao{};
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        buffers_object->vao = vao;
        buffers_object->buffers.reserve(count);

        bool element_array_bound = false;
        size_t data_point_count = (size_t)-1;

        for (size_t i = 0; i < count; i++) {
            const auto& buf = buffers[i];
            if (buf->is_element_array) {
                if (element_array_bound) {
                    log.error("Only one index buffer is allowed per buffers object");
                    delete buffers_object;
                    return nullptr;
                }
                element_array_bound = true;
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf->buffer);
                buffers_object->has_index_buffer = true;
            }
            else
                glBindBuffer(GL_ARRAY_BUFFER, buf->buffer);
            glEnableVertexAttribArray(i);
            glVertexAttribPointer(i, buf->gl_data_type_point_count, buf->gl_data_type, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(i);
            buffers_object->buffers.emplace_back(buf);

            if (data_point_count == (size_t)-1 || buf->is_element_array)
                data_point_count = buf->size;
            else if (data_point_count != buf->size) {
                log.error("Buffers in buffers object have different sizes");
                delete buffers_object;
                return nullptr;
            }
        }

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        return buffers_object;
    }

    EXPORT void destroy_buffers_object(BackendData*, BuffersObject* buffers_object) {
        delete buffers_object;
    }

    EXPORT Shader* create_shader(BackendData*, const ShaderCreateInfo& info) {
        GLuint prog = glCreateProgram();
        Shader* shader = new Shader{};

        for (const auto& shader_info : info.shader_sources) {
            GLuint shader_type{};
            switch (shader_info.type) {
                case ShaderCreateInfo::SingleShaderInfo::Type::VERTEX:
                    shader_type = GL_VERTEX_SHADER;
                    shader->type = Shader::Type::GRAPHICS;
                    break;
                case ShaderCreateInfo::SingleShaderInfo::Type::FRAGMENT:
                    shader_type = GL_FRAGMENT_SHADER;
                    shader->type = Shader::Type::GRAPHICS;
                    break;
                case ShaderCreateInfo::SingleShaderInfo::Type::COMPUTE:
                    shader_type = GL_COMPUTE_SHADER;
                    shader->type = Shader::Type::COMPUTE;
                    break;
            }

            GLuint shader_id = glCreateShader(shader_type);
            const char* source = shader_info.source.c_str();
            glShaderSource(shader_id, 1, &source, nullptr);
            glCompileShader(shader_id);

            GLint success;
            glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
            if (!success) {
                char info_log[512];
                glGetShaderInfoLog(shader_id, 512, nullptr, info_log);
                log.error("Shader compilation failed: {}", info_log);
                delete shader;
                return nullptr;
            }

            glAttachShader(prog, shader_id);
            glDeleteShader(shader_id);
        }

        glLinkProgram(prog);
        GLint success;
        glGetProgramiv(prog, GL_LINK_STATUS, &success);
        if (!success) {
            char info_log[512];
            glGetProgramInfoLog(prog, 512, nullptr, info_log);
            log.error("Shader linking failed: {}", info_log);
            delete shader;
            return nullptr;
        }

        shader->prog = prog;
        return shader;
    }
    
    EXPORT void destroy_shader(BackendData*, Shader* shader) {
        delete shader;
    }

    EXPORT Texture* create_texture(BackendData*, const TextureCreateInfo& info) {
        GLuint tex{};
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        
        GLint internal_format{};
        switch (info.num_channels) {
            case 1: internal_format = GL_RED; break;
            case 2: internal_format = GL_RG; break;
            case 3: internal_format = GL_RGB; break;
            case 4: internal_format = GL_RGBA; break;
            default: {
                log.error("Invalid number of channels for texture (must be 1-4)");
                glDeleteTextures(1, &tex);
                return nullptr;
            }
        };
        GLint channel_size{};
        switch (info.channel_size_in_bytes) {
            case 1: channel_size = GL_UNSIGNED_BYTE; break;
            case 2: channel_size = GL_UNSIGNED_SHORT; break;
            case 4: channel_size = GL_FLOAT; break;
            default: {
                log.error("Invalid channel size for texture (must be 1, 2, or 4)");
                glDeleteTextures(1, &tex);
                return nullptr;
            }
        };

        glTexImage2D(
            GL_TEXTURE_2D, 0, internal_format, info.size.x(), info.size.y(), 0,
            internal_format, channel_size, info.data
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);

        return new Texture{
            .tex = tex,
            .size = info.size
        };
    }

    EXPORT void destroy_texture(BackendData*, Texture* texture) {
        delete texture;
    }

    EXPORT void push_draw_call(BackendData* backend, Shader* shader, BuffersObject* buffers_object, Texture** textures, size_t num_textures, const std::unordered_map<std::string, std::any>& parameters) {
        backend->draw_calls.emplace_back(BackendData::DrawCall{
            .shader = shader,
            .buffers_object = buffers_object,
            .textures = std::vector<Texture*>(textures, textures + num_textures),
            .parameters = parameters
        });
    }



    //====================================
    // Backend initialization and freeing
    //====================================

    EXPORT BackendData* create_backend(NativeWindow* native_window) {
        if (initialized.exchange(true)) {
            log.error("OpenGL backend only supports one instance at a time");
            return nullptr;
        }

        BackendData* data = new BackendData{};
        data->platform = new OpenGLPlatform{false};
        data->platform->create_context(4, 6, native_window);
        data->platform->make_current();

        if (!gladLoadGLLoader((GLADloadproc)OpenGLPlatform::proc_address_getter)) {
	        log.error("Could not load GLAD");
            delete data->platform;
            delete data;
            return nullptr;
        }

        log.log("Initialized OpenGL Backend");
        data->init = true;

        return data;
    }

    EXPORT void destroy_backend(BackendData* data) {
        log.log("Destroyed OpenGL Backend");
        data->init = false;
        delete data->platform;
        delete data;
        initialized = false;
    }
}
