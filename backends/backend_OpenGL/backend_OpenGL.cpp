#include "backend.hpp"
#include "backend_OpenGL.hpp"
#include "glad/glad.h"
#include "shaders.hpp"
#include <atomic>
#include <iostream>
#include <mutex>


namespace mgm {
    struct BuffersObject;

    struct Buffer {
        GLuint buffer = 0;
        GLint gl_data_type = 0;
        GLint bind_location = 0;
        size_t gl_data_type_point_count = 0;
        size_t size = 0;
        size_t data_point_size = 0;
        BuffersObject* connected_to = nullptr;
        bool is_element_array: 1 = false;

        ~Buffer() {
            if (buffer)
                glDeleteBuffers(1, &buffer);
        }
    };

    struct Shader {
        enum class Type {
            GRAPHICS, COMPUTE
        } type = Type::GRAPHICS;
        GLuint prog = 0;
        std::unordered_map<std::string, GLint> uniform_locations{};

        ~Shader() {
            if (prog)
                glDeleteProgram(prog);
        }
    };

    struct BuffersObject {
        std::unordered_map<std::string, Buffer*> buffers{};
        size_t size = 0;
        GLuint vao = 0;
        bool has_index_buffer = false;
        Shader* last_used_shader = nullptr;

        ~BuffersObject() {
            if (vao)
                glDeleteVertexArrays(1, &vao);
        }
    };

    struct Texture {
        std::string name{};
        GLuint tex = 0;
        GLuint fbo = (GLuint)-1;
        GLuint rbo = (GLuint)-1;
        vec2i32 size{};

        GLint internal_format = GL_RGBA;
        GLint channel_size = GL_UNSIGNED_BYTE;

        ~Texture() {
            if (tex)
                glDeleteTextures(1, &tex);
            if (fbo) {
                glDeleteFramebuffers(1, &fbo);
                glDeleteRenderbuffers(1, &rbo);
            }
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

        Texture* canvas = nullptr;
        
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
    std::mutex mutex{};



    //============================
    // Backend graphics functions
    //============================

    GLenum gl_blending_factor(GPUSettings::Blending::Factor factor) {
        switch (factor) {
            case GPUSettings::Blending::Factor::ZERO: return GL_ZERO;
            case GPUSettings::Blending::Factor::ONE: return GL_ONE;
            case GPUSettings::Blending::Factor::SRC_COLOR: return GL_SRC_COLOR;
            case GPUSettings::Blending::Factor::ONE_MINUS_SRC_COLOR: return GL_ONE_MINUS_SRC_COLOR;
            case GPUSettings::Blending::Factor::DST_COLOR: return GL_DST_COLOR;
            case GPUSettings::Blending::Factor::ONE_MINUS_DST_COLOR: return GL_ONE_MINUS_DST_COLOR;
            case GPUSettings::Blending::Factor::SRC_ALPHA: return GL_SRC_ALPHA;
            case GPUSettings::Blending::Factor::ONE_MINUS_SRC_ALPHA: return GL_ONE_MINUS_SRC_ALPHA;
            case GPUSettings::Blending::Factor::DST_ALPHA: return GL_DST_ALPHA;
            case GPUSettings::Blending::Factor::ONE_MINUS_DST_ALPHA: return GL_ONE_MINUS_DST_ALPHA;
        }
        return GL_INVALID_ENUM;
    }
    GLenum gl_blending_equation(GPUSettings::Blending::Equation equation) {
        switch (equation) {
            case GPUSettings::Blending::Equation::ADD: return GL_FUNC_ADD;
            case GPUSettings::Blending::Equation::SRC_MINUS_DST: return GL_FUNC_SUBTRACT;
            case GPUSettings::Blending::Equation::DST_MINUS_SRC: return GL_FUNC_REVERSE_SUBTRACT;
            case GPUSettings::Blending::Equation::MIN: return GL_MIN;
            case GPUSettings::Blending::Equation::MAX: return GL_MAX;
        }
        return GL_INVALID_ENUM;
    }

    EXPORT bool set_attribute(BackendData* backend, const GPUSettings::StateAttribute& attr, const void* data) {
        mutex.lock();
        backend->platform->make_current();

        // Unused until one of the attributes has a failure condition
        constexpr bool success = true;

        switch (attr) {
            case GPUSettings::StateAttribute::CLEAR: {
                const auto& clear = *static_cast<const GPUSettings::GPUSettings::Clear*>(data);
                backend->clear_color = clear.color;
                backend->clear_mask = clear.color_buffer * GL_COLOR_BUFFER_BIT
                    | clear.depth_buffer * GL_DEPTH_BUFFER_BIT
                    | clear.stencil_buffer * GL_STENCIL_BUFFER_BIT;
                glClearColor(clear.color.x(), clear.color.y(), clear.color.z(), clear.color.w());
                glClearDepth(1.0);
                glClearStencil(0);
                break;
            }
            case GPUSettings::StateAttribute::BLENDING: {
                const auto& blending = *static_cast<const GPUSettings::GPUSettings::Blending*>(data);
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
                break;
            }
            case GPUSettings::StateAttribute::VIEWPORT: {
                const auto& viewport = *static_cast<const GPUSettings::GPUSettings::Viewport*>(data);
                backend->viewport.pos = viewport.top_left;
                backend->viewport.size = viewport.bottom_right - viewport.top_left;
                glViewport(
                    backend->viewport.pos.x(), backend->viewport.pos.y(),
                    backend->viewport.size.x(), backend->viewport.size.y()
                );
                break;
            }
            case GPUSettings::StateAttribute::SCISSOR: {
                const auto& scissor = *static_cast<const GPUSettings::GPUSettings::Scissor*>(data);
                if ((scissor.top_left == vec2i32{0, 0} && scissor.bottom_right == vec2i32{0, 0}) || !scissor.enabled)
                    glDisable(GL_SCISSOR_TEST);
                else {
                    backend->scissor.pos = scissor.top_left;
                    backend->scissor.size = scissor.bottom_right - scissor.top_left;
                    glEnable(GL_SCISSOR_TEST);
                    glScissor(
                        backend->scissor.pos.x(), backend->scissor.pos.y(),
                        backend->scissor.size.x(), backend->scissor.size.y()
                    );
                }
                break;
            }
        }

        backend->platform->make_null_current();
        mutex.unlock();
        return success;
    }

    EXPORT void clear(BackendData* backend) {
        mutex.lock();
        if (backend->clear_mask) {
            backend->platform->make_current();
            glClear(backend->clear_mask);
            backend->platform->make_null_current();
        }
        mutex.unlock();
    }

    void set_uniform(const GLint uniform, const std::any& value) {
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
            glUniformMatrix2fv(uniform, 1, GL_FALSE, (const float*)&mat.data);
        }
        else if (value.type().hash_code() == typeid(mat3f).hash_code()) {
            const auto& mat = std::any_cast<mat3f>(value);
            glUniformMatrix3fv(uniform, 1, GL_FALSE, (const float*)&mat.data);
        }
        else if (value.type().hash_code() == typeid(mat4f).hash_code()) {
            const auto& mat = std::any_cast<mat4f>(value);
            glUniformMatrix4fv(uniform, 1, GL_FALSE, (const float*)&mat.data);
        }
        else
            log.error("Unsupported shader parameter type '", value.type().name(), "'");
    }

    void make_texture_canvas(Texture* tex);
    void setup_vao_attrib_pointers(BuffersObject* buffers);

    EXPORT void execute(BackendData* backend, Texture* canvas) {
        mutex.lock();

        if (backend->draw_calls.empty()) {
            mutex.unlock();
            return;
        }

        backend->platform->make_current();
        if (canvas) {
            if (canvas != backend->canvas) {
                if (backend->viewport.size.x() > canvas->size.x() || backend->viewport.size.y() > canvas->size.y()) {
                    log.error("Viewport size is larger than canvas size");
                    backend->platform->make_null_current();
                    mutex.unlock();
                    return;
                }
                make_texture_canvas(canvas);
                glBindFramebuffer(GL_FRAMEBUFFER, canvas->fbo);
                backend->canvas = canvas;
            }
        }
        else if (backend->canvas) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            backend->canvas = nullptr;
        }

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
                    if (!draw_call.textures[i]->name.empty()) {
                        const auto& name = draw_call.textures[i]->name;
                        const auto it = draw_call.shader->uniform_locations.find(name);
                        if (it == draw_call.shader->uniform_locations.end()) {
                            const auto loc = glGetUniformLocation(draw_call.shader->prog, name.c_str());
                            if (loc == -1) {
                                log.error("Could not find texture '", name, "' in shader");
                                continue;
                            }
                            draw_call.shader->uniform_locations[name] = loc;
                            glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(loc - 1));
                            glBindTexture(GL_TEXTURE_2D, draw_call.textures[i]->tex);
                        }
                        else {
                            glActiveTexture((GLenum)it->second);
                            glBindTexture(GL_TEXTURE_2D, draw_call.textures[i]->tex);
                        }
                    }
                    else {
                        glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(i));
                        glBindTexture(GL_TEXTURE_2D, draw_call.textures[i]->tex);
                    }
                }
            }

            if (draw_call.buffers_object->last_used_shader != draw_call.shader) {
                draw_call.buffers_object->last_used_shader = draw_call.shader;
                setup_vao_attrib_pointers(draw_call.buffers_object);
            }

            glBindVertexArray(draw_call.buffers_object->vao);
            if (draw_call.buffers_object->has_index_buffer)
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(draw_call.buffers_object->size), GL_UNSIGNED_INT, nullptr);
            else
                glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(draw_call.buffers_object->size));
        }
        backend->draw_calls.clear();

        backend->platform->make_null_current();
        mutex.unlock();
    }

    EXPORT void present(BackendData* backend) {
        mutex.lock();
        backend->platform->make_current();
        backend->platform->swap_buffers();
        backend->platform->make_null_current();
        mutex.unlock();
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

    EXPORT void* create_buffer(BackendData* backend, const BufferCreateInfo& info) {
        const auto gl_buffer_type = info.type() == BufferCreateInfo::Type::RAW ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER;
        if (gl_buffer_type == GL_INVALID)
            return nullptr;
        GLuint buf{};

        mutex.lock();
        backend->platform->make_current();
        glGenBuffers(1, &buf);
        backend->platform->make_null_current();
        mutex.unlock();

        Buffer* buffer = new Buffer{
            .buffer = buf,
            .gl_data_type = gl_type_id(info.type_id_hash()),
            .gl_data_type_point_count = gl_type_point_count(info.type_id_hash()),
            .size = info.size(),
            .data_point_size = info.data_point_size(),
            .is_element_array = info.type() == BufferCreateInfo::Type::INDEX
        };
        
        buffer_data(backend, buffer, info.data(), info.size());
        return buffer;
    }

    EXPORT void buffer_data(BackendData* backend, Buffer* buffer, void* data, size_t size) {
        const GLenum gl_buffer_type = buffer->is_element_array ? GL_ELEMENT_ARRAY_BUFFER : GL_ARRAY_BUFFER;
        mutex.lock();
        backend->platform->make_current();

        glBindBuffer(gl_buffer_type, buffer->buffer);
        glBufferData(gl_buffer_type, (GLsizeiptr)(size * buffer->data_point_size), data, GL_STATIC_DRAW);
        glBindBuffer(gl_buffer_type, 0);
        buffer->size = size;
        buffer->bind_location = -1;

        backend->platform->make_null_current();
        mutex.unlock();
    }

    EXPORT void destroy_buffer(BackendData* backend, Buffer* buffer) {
        mutex.lock();
        backend->platform->make_current();
        delete buffer;
        backend->platform->make_null_current();
        mutex.unlock();
    }

    void setup_vao_attrib_pointers(BuffersObject* buffers) {
        Buffer* ebo = nullptr;

        size_t data_point_count = (size_t)-1;

        for (const auto& [name, buf] : buffers->buffers) {
            if (buf->is_element_array) {
                if (ebo) {
                    log.error("Only one index buffer is allowed per buffers object");
                    return;
                }
                ebo = buf;
                data_point_count = buf->size;
                continue;
            }

            if (data_point_count == (size_t)-1)
                data_point_count = buf->size;
            else if (data_point_count != buf->size && !ebo) {
                log.error("Buffers in buffers object have different sizes");
                return;
            }
        }

        glBindVertexArray(buffers->vao);

        for (const auto& [name, buf] : buffers->buffers) {
            if (buf == ebo)
                continue;

            const auto loc = glGetAttribLocation(buffers->last_used_shader->prog, name.c_str());

            if (loc == GL_INVALID) {
                log.error("No buffer by the name \"", name, "\"");
                return;
            }

            glBindBuffer(GL_ARRAY_BUFFER, buf->buffer);
            glVertexAttribPointer(static_cast<GLuint>(loc), static_cast<GLint>(buf->gl_data_type_point_count), (GLenum)buf->gl_data_type, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(static_cast<GLuint>(loc));
            buf->bind_location = loc;
        }
        if (ebo) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo->buffer);
            buffers->has_index_buffer = true;
        }

        buffers->size = data_point_count;

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    EXPORT BuffersObject* create_buffers_object(BackendData* backend, Buffer** buffers, const std::string* names, size_t count) {
        const auto buffers_object = new BuffersObject{};

        mutex.lock();
        backend->platform->make_current();

        glGenVertexArrays(1, &buffers_object->vao);
        buffers_object->buffers.clear();
        for (size_t i = 0; i < count; ++i)
            buffers_object->buffers.emplace(names[i], buffers[i]);

        backend->platform->make_null_current();
        mutex.unlock();

        return buffers_object;
    }

    EXPORT void destroy_buffers_object(BackendData* backend, BuffersObject* buffers_object) {
        mutex.lock();
        backend->platform->make_current();
        delete buffers_object;
        backend->platform->make_null_current();
        mutex.unlock();
    }

    struct _GLSLSources {
        std::string vertex{}, fragment{};
    };
    std::string generate_func_body(MgmGPUShaderBuilder& builder, MgmGPUShaderBuilder::Function& func, const std::string& func_name) {
        std::string func_body{};

        for (auto& line : func.lines) {
            auto& ops = line.operations;
            for (size_t i = 0; i < ops.size(); ++i) {
                const auto it = std::find(func.all_ops.begin(), func.all_ops.end(), ops[i]);
                if (it != func.all_ops.end()) {
                    if (*it == "()") {
                        const auto arg_c = std::stoull(ops[i - 1]);
                        if (arg_c == 1 && ops[i - 2].empty()) {
                            const auto cont = ops[i - 3];
                            i -= arg_c + 2;
                            ops.erase(ops.begin() + (long)i, ops.begin() + (long)(i + arg_c + 3));
                            ops.insert(ops.begin() + (long)i, cont);
                        }
                        else {
                            std::string op_res = ops[i - 2] + '(';
                            for (size_t j = i - arg_c - 2; j < i - 2; ++j) {
                                op_res += ops[j];
                                if (arg_c > 1 && j < i - 3)
                                    op_res += ',';
                            }
                            op_res += ')';
                            i -= arg_c + 2;
                            ops.erase(ops.begin() + (long)i, ops.begin() + (long)(i + arg_c + 3));
                            ops.insert(ops.begin() + (long)i, op_res);
                        }
                    }
                    else if (*it == "[]") {
                        std::string op_res{};
                        if (builder.textures.contains(ops[i - 2]))
                            op_res = "texture(" + ops[i - 2] + ", " + ops[i - 1] + ")";
                        else
                            op_res = ops[i - 2] + '[' + ops[i - 1] + ']';
                        i -= 2;
                        ops.erase(ops.begin() + (long)i, ops.begin() + (long)(i + 3));
                        ops.insert(ops.begin() + (long)i, op_res);
                    }
                    else {
                        const auto op_res = ops[i - 2] + ops[i] + ops[i - 1];
                        i -= 2;
                        ops.erase(ops.begin() + (long)i, ops.begin() + (long)(i + 3));
                        ops.insert(ops.begin() + (long)i, op_res);
                    }
                }
                else if (ops[i] == "return") {
                    if (ops.size() != 2)
                        return "";

                    if (func_name == "vertex")
                        func_body += "gl_Position = " + ops[0] + ";\nreturn;\n";
                    else if (func_name == "pixel")
                        func_body += "out_FragColor = " + ops[0] + ";\nreturn;\n";
                    else
                        func_body += "return " + ops[0] + ";\n";
                    ops = {};
                }
                else if (ops[i] == "var") {
                    if (ops.size() != 4)
                        return "";
                    func_body += ops[2] + ' ' + ops[1];
                    if (ops[0].empty())
                        func_body += ";\n";
                    else
                        func_body += " = " + ops[0] + ";\n";
                    ops = {};
                }
            }

            if (!line.state.empty()) {
                if (line.state.ends_with("if"))
                    func_body += "if (";
                else if (line.state.ends_with("while"))
                    func_body += "while (";

                func_body += line.operations[0] + ")\n{\n" + generate_func_body(builder, builder.pseudo_functions.at(line.state), line.state) + "}\n";
                continue;
            }

            if (ops.size() > 1)
                return "";
            else if (ops.size() == 1)
                func_body += ops.front() + ";\n";
        }

        return func_body;
    }
    _GLSLSources make_glsl_from_builder(MgmGPUShaderBuilder& builder) {
        if (!builder.errors.empty()) {
            for (const auto& error : builder.errors)
                Logging{"Shader Builder"}.error(error.message, " at ", std::to_string(error.line), ":", std::to_string(error.column));
            return {};
        }

        std::string vertex{"#version 460 core\n\n"};
        std::string fragment{"#version 460 core\n\n"};
        
        const auto vit = builder.functions.find("vertex");
        if (vit != builder.functions.end()) {
            for (const auto& [param, type] : vit->second.function_parameters) {
                vertex += "in " + type + " " + param + ";\n";
            }
        }

        vertex += '\n';

        for (const auto& [name, parameter] : builder.parameters) {
            vertex += "uniform " + parameter.type_name + ' ' + name + ";\n";
            fragment += "uniform " + parameter.type_name + ' ' + name + ";\n";
        }

        vertex += '\n';
        fragment += '\n';

        for (const auto& [name, texture] : builder.textures)
            fragment += "uniform sampler" + std::to_string(texture.dimensions) + "D " + name + ";\n";
        
        fragment += '\n';

        const auto pit = builder.functions.find("pixel");
        if (pit != builder.functions.end()) {
            for (const auto& [param, type] : pit->second.function_parameters) {
                vertex += "out " + type + ' ' + param + ";\n";
                fragment += "in " + type + ' ' + param + ";\n";
            }
        }

        vertex += '\n';
        fragment += "\n\nout vec4 out_FragColor;\n\n";

        for (auto& [name, function] : builder.functions) {
            const auto func_body = generate_func_body(builder, function, name);
            if (name == "vertex")
                vertex += "void main() {\n" + func_body + "}\n";
            else if (name == "pixel")
                fragment += "void main() {\n" + func_body + "}\n";
        }

        return {
            .vertex = vertex,
            .fragment = fragment
        };
    }

    EXPORT Shader* create_shader(BackendData* backend, const MgmGPUShaderBuilder& original_builder) {
        std::unique_lock lock{mutex};
        backend->platform->make_current();
        GLuint prog = glCreateProgram();

        Shader* shader = new Shader{};

        auto builder = original_builder;

        if (builder.functions.contains("compute")) {
            shader->type = Shader::Type::COMPUTE;
            // TODO: Implement compute shaders
        }
        else if (builder.functions.contains("vertex") && builder.functions.contains("pixel")) {
            shader->type = Shader::Type::GRAPHICS;

            const auto glsl_source = make_glsl_from_builder(builder);

            std::cout << glsl_source.vertex << "\n\n" << glsl_source.fragment << std::endl;

            GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
            const char* source = glsl_source.vertex.c_str();
            glShaderSource(vertex_shader_id, 1, &source, nullptr);
            glCompileShader(vertex_shader_id);

            GLint success;
            glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &success);
            if (!success) {
                char info_log[512];
                glGetShaderInfoLog(vertex_shader_id, 512, nullptr, info_log);
                log.error("Shader compilation failed: ", info_log);
                delete shader;
                backend->platform->make_null_current();
                return nullptr;
            }

            GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
            source = glsl_source.fragment.c_str();
            glShaderSource(fragment_shader_id, 1, &source, nullptr);
            glCompileShader(fragment_shader_id);

            glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &success);
            if (!success) {
                char info_log[512];
                glGetShaderInfoLog(fragment_shader_id, 512, nullptr, info_log);
                log.error("Shader compilation failed: ", info_log);
                delete shader;
                backend->platform->make_null_current();
                return nullptr;
            }

            glAttachShader(prog, vertex_shader_id);
            glDeleteShader(vertex_shader_id);
            glAttachShader(prog, fragment_shader_id);
            glDeleteShader(fragment_shader_id);
        }

        glLinkProgram(prog);
        GLint success;
        glGetProgramiv(prog, GL_LINK_STATUS, &success);
        if (!success) {
            char info_log[512];
            glGetProgramInfoLog(prog, 512, nullptr, info_log);
            log.error("Shader linking failed: {}", info_log);
            delete shader;
            backend->platform->make_null_current();
            return nullptr;
        }
        backend->platform->make_null_current();

        shader->prog = prog;
        return shader;
    }
    
    EXPORT void destroy_shader(BackendData* backend, Shader* shader) {
        mutex.lock();
        backend->platform->make_current();
        delete shader;
        backend->platform->make_null_current();
        mutex.unlock();
    }

    EXPORT Texture* create_texture(BackendData* backend, const TextureCreateInfo& info) {
        GLuint tex{};

        mutex.lock();
        backend->platform->make_current();

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
                backend->platform->make_null_current();
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
                backend->platform->make_null_current();
                return nullptr;
            }
        };

        glTexImage2D(
            GL_TEXTURE_2D, 0, internal_format, info.size.x(), info.size.y(), 0,
            (GLenum)internal_format, (GLenum)channel_size, info.data
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);

        backend->platform->make_null_current();
        mutex.unlock();

        return new Texture{
            .name = info.name,
            .tex = tex,
            .size = info.size,
            .internal_format = internal_format,
            .channel_size = channel_size
        };
    }

    void make_texture_canvas(Texture* tex) {
        if (tex->fbo != (GLuint)-1)
            return;

        GLuint fbo{}, rbo{};
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, tex->size.x(), tex->size.y());
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->tex, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            log.error("Failed to make texture a canvas, OpenGL framebuffer is incomplete");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        tex->fbo = fbo;
        tex->rbo = rbo;
    }

    EXPORT void destroy_texture(BackendData* backend, Texture* texture) {
        mutex.lock();
        backend->platform->make_current();
        delete texture;
        backend->platform->make_null_current();
        mutex.unlock();
    }

    EXPORT void push_draw_call(BackendData* backend, Shader* shader, BuffersObject* buffers_object, Texture** textures, size_t num_textures, const std::unordered_map<std::string, std::any>& parameters) {
        mutex.lock();
        backend->draw_calls.emplace_back(BackendData::DrawCall{
            .shader = shader,
            .buffers_object = buffers_object,
            .textures = std::vector<Texture*>(textures, textures + num_textures),
            .parameters = parameters
        });
        mutex.unlock();
    }



    //====================================
    // Backend initialization and freeing
    //====================================

    EXPORT BackendData* create_backend(NativeWindow* native_window) {
        if (initialized.exchange(true)) {
            log.error("OpenGL backend only supports one instance at a time");
            return nullptr;
        }

        mutex.lock();

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
        const char* vendor = (const char*)glGetString(GL_VENDOR);
        const char* renderer = (const char*)glGetString(GL_RENDERER);
        const char* version = (const char*)glGetString(GL_VERSION);
        log.log("\tOpenGL Vendor: ", vendor);
        log.log("\tOpenGL Renderer: ", renderer);
        log.log("\tOpenGL Version: ", version);
        data->init = true;

        data->platform->make_null_current();
        mutex.unlock();

        return data;
    }

    EXPORT void destroy_backend(BackendData* backend) {
        mutex.lock();

        backend->init = false;
        delete backend->platform;
        delete backend;
        initialized = false;

        mutex.unlock();

        log.log("Destroyed OpenGL Backend");
    }
}
