#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "backend_settings.hpp"
#include "mgmgpu.hpp"

#include "mgmwin.hpp"
#include "shaders.hpp"

#if defined(EMBED_BACKEND)
#include "backend.hpp"
#else
#include "dloader.hpp"
#endif


namespace mgm {
    template<typename T>
    class SimpleSparseSet {
        std::unordered_map<ID_t, T> map{};
        ID_t place = ID_t{0};

      public:
        template<typename... Ts>
        ID_t create(Ts&&... args) {
            const auto p = place++;
            map.emplace(p, T{std::forward<Ts>(args)...});
            return p;
        }

        void destroy(ID_t id) {
            const auto it = map.find(id);
            if (it == map.end())
                return;

            map.erase(it);
        }

        T& operator[](ID_t id) { return map.at(id); }

        const T& operator[](ID_t id) const { return map.at(id); }

        bool check(const ID_t id) const { return map.find(id) != map.end(); }
    };

    struct Buffer;
    struct BuffersObject;
    struct Shader;
    struct Texture;

    struct MgmGPU::Data {
        GPUSettings old_settings{};
        bool initialized = false;

        struct BackendData;
        BackendData* backend = nullptr;

        using CreateBackend = BackendData* (*)(NativeWindow* window);
        using DestroyBackend = void (*)(BackendData* backend);
        using SetAttribute = bool (*)(BackendData* backend, const GPUSettings::StateAttribute& attr, const void* data);

        using Clear = void (*)(BackendData* backend, Texture* canvas);
        using Execute = void (*)(BackendData* backend, Texture* canvas);
        using Present = void (*)(BackendData* backend);

        using CreateBuffer = Buffer* (*)(BackendData* backend, const BufferCreateInfo& info);
        using BufferData = void (*)(BackendData* backend, Buffer* buffer, const void* data, size_t size);
        using DestroyBuffer = void (*)(BackendData* backend, Buffer* buffer);
        using CreateBuffersObject = BuffersObject* (*)(BackendData* backend, Buffer** buffers, const std::string* names,
                                                       size_t count);
        using DestroyBuffersObject = void (*)(BackendData* backend, BuffersObject* buffers_object);
        using CreateShader = Shader* (*)(BackendData* backend, const MgmGPUShaderBuilder& info);
        using DestroyShader = void (*)(BackendData* backend, Shader* shader);
        using CreateTexture = Texture* (*)(BackendData* backend, const TextureCreateInfo& info);
        using DestroyTexture = void (*)(BackendData* backend, Texture* texture);

        using PushDrawCall = void (*)(BackendData* backend, Shader* shader, BuffersObject* buffers_object, Texture** textures, size_t num_textures, const std::unordered_map<std::string, std::any>& parameters);

        bool loaded = false;
        CreateBackend create_backend{};
        DestroyBackend destroy_backend{};
        SetAttribute set_attribute{};

        Clear clear{};
        Execute execute{};
        Present present{};

        CreateBuffer create_buffer{};
        BufferData buffer_data{};
        DestroyBuffer destroy_buffer{};
        CreateBuffersObject create_buffers_object{};
        DestroyBuffersObject destroy_buffers_object{};
        CreateShader create_shader{};
        DestroyShader destroy_shader{};
        CreateTexture create_texture{};
        DestroyTexture destroy_texture{};

        PushDrawCall push_draw_call{};

        struct RawBufferInfo {
            Buffer* buffer = nullptr;
            BufferCreateInfo original_info{};
        };

        SimpleSparseSet<RawBufferInfo> buffers{};
        SimpleSparseSet<BuffersObject*> buffers_objects{};
        SimpleSparseSet<Shader*> shaders{};
        SimpleSparseSet<Texture*> textures{};

        struct StateAttributeOffset {
            size_t offset{};
            size_t size{};
        };
        std::unordered_map<GPUSettings::StateAttribute, StateAttributeOffset> settings_offsets{};

#if !defined(EMBED_BACKEND)
        DLoader dloader{};
#endif

        std::mutex mutex{};

        Logging log{"MgmGPU"};
    };


    MgmGPU::MgmGPU(MgmWindow* window_to_connect) {
        data = new Data{};
        GPUSettings backend_settings{};
        data->settings_offsets.emplace(
            GPUSettings::StateAttribute::CLEAR,
            Data::StateAttributeOffset{(size_t)&backend_settings.clear - (size_t)&backend_settings, sizeof(GPUSettings::Clear)}
        );
        data->settings_offsets.emplace(
            GPUSettings::StateAttribute::DEPTH,
            Data::StateAttributeOffset{(size_t)&backend_settings.depth_testing - (size_t)&backend_settings, sizeof(GPUSettings::Depth)}
        );
        data->settings_offsets.emplace(
            GPUSettings::StateAttribute::CULLING,
            Data::StateAttributeOffset{(size_t)&backend_settings.culling - (size_t)&backend_settings, sizeof(GPUSettings::Culling)}
        );
        data->settings_offsets.emplace(
            GPUSettings::StateAttribute::BLENDING,
            Data::StateAttributeOffset{(size_t)&backend_settings.blending - (size_t)&backend_settings, sizeof(GPUSettings::Blending)}
        );
        data->settings_offsets.emplace(
            GPUSettings::StateAttribute::VIEWPORT,
            Data::StateAttributeOffset{(size_t)&backend_settings.viewport - (size_t)&backend_settings, sizeof(GPUSettings::Viewport)}
        );
        data->settings_offsets.emplace(
            GPUSettings::StateAttribute::SCISSOR,
            Data::StateAttributeOffset{(size_t)&backend_settings.scissor - (size_t)&backend_settings, sizeof(GPUSettings::Scissor)}
        );

        connect_to_window(window_to_connect);
    }

    void MgmGPU::apply_settings(const GPUSettings& backend_settings) {
        if (!is_backend_loaded())
            return;

        for (const auto& [attr, offset] : data->settings_offsets) {
            const auto changed = data->initialized
                                     ? true
                                     : (std::memcmp(reinterpret_cast<const char*>(&backend_settings) + offset.offset, reinterpret_cast<const char*>(&data->old_settings) + offset.offset, offset.size) != 0);

            if (changed)
                data->set_attribute(data->backend, attr, reinterpret_cast<const char*>(&backend_settings) + offset.offset);
        }

        if (!data->initialized)
            data->initialized = true;

        data->old_settings = backend_settings;
    }

    void MgmGPU::connect_to_window(MgmWindow* window_to_connect) {
        if (window) {
            data->log.warning("Already connected to a window, disconnecting first");
            disconnect_from_window();
        }
        window = window_to_connect;
    }

    void MgmGPU::disconnect_from_window() {
        if (data->loaded) {
            data->log.warning("Disconnecting from window while backend is loaded, unloading backend");
            unload_backend();
        }
        window = nullptr;
    }

    void MgmGPU::load_backend(const Path& path) {
        if (data->loaded) {
            data->log.warning("A backend is already loaded, unloading it first");
            unload_backend();
        }
        if (!window) {
            data->log.error("Backend not connected to a window");
            return;
        }

#if defined(EMBED_BACKEND)
        if (!path.empty())
            data->log.warning("Ignoring backend at path \"", path, "\" when using embedded backend");

        data->create_backend = reinterpret_cast<decltype(data->create_backend)>(&mgm::create_backend);
        data->destroy_backend = reinterpret_cast<decltype(data->destroy_backend)>(&mgm::destroy_backend);
        data->set_attribute = reinterpret_cast<decltype(data->set_attribute)>(&mgm::set_attribute);

        data->clear = reinterpret_cast<decltype(data->clear)>(&mgm::clear);
        data->execute = reinterpret_cast<decltype(data->execute)>(&mgm::execute);
        data->present = reinterpret_cast<decltype(data->present)>(&mgm::present);

        data->create_buffer = reinterpret_cast<decltype(data->create_buffer)>(&mgm::create_buffer);
        data->buffer_data = reinterpret_cast<decltype(data->buffer_data)>(&mgm::buffer_data);
        data->destroy_buffer = reinterpret_cast<decltype(data->destroy_buffer)>(&mgm::destroy_buffer);
        data->create_buffers_object = reinterpret_cast<decltype(data->create_buffers_object)>(&mgm::create_buffers_object);
        data->destroy_buffers_object = reinterpret_cast<decltype(data->destroy_buffers_object)>(&mgm::destroy_buffers_object);
        data->create_shader = reinterpret_cast<decltype(data->create_shader)>(&mgm::create_shader);
        data->destroy_shader = reinterpret_cast<decltype(data->destroy_shader)>(&mgm::destroy_shader);
        data->create_texture = reinterpret_cast<decltype(data->create_texture)>(&mgm::create_texture);
        data->destroy_texture = reinterpret_cast<decltype(data->destroy_texture)>(&mgm::destroy_texture);

        data->push_draw_call = reinterpret_cast<decltype(data->push_draw_call)>(&mgm::push_draw_call);
#else
        data->dloader.load(path.platform_path().c_str());

        if (!data->dloader.is_loaded()) {
            data->log.error("Failed to load backend");
            return;
        }

        data->dloader.sym("create_backend", &data->create_backend);
        data->dloader.sym("destroy_backend", &data->destroy_backend);
        data->dloader.sym("set_attribute", &data->set_attribute);

        data->dloader.sym("clear", &data->clear);
        data->dloader.sym("execute", &data->execute);
        data->dloader.sym("present", &data->present);

        data->dloader.sym("create_buffer", &data->create_buffer);
        data->dloader.sym("buffer_data", &data->buffer_data);
        data->dloader.sym("destroy_buffer", &data->destroy_buffer);
        data->dloader.sym("create_buffers_object", &data->create_buffers_object);
        data->dloader.sym("destroy_buffers_object", &data->destroy_buffers_object);
        data->dloader.sym("create_shader", &data->create_shader);
        data->dloader.sym("destroy_shader", &data->destroy_shader);
        data->dloader.sym("create_texture", &data->create_texture);
        data->dloader.sym("destroy_texture", &data->destroy_texture);

        data->dloader.sym("push_draw_call", &data->push_draw_call);
#endif

        data->loaded = true;

        data->backend = data->create_backend(window->get_native_window());

        apply_settings(GPUSettings{});

        data->log.log("Loaded backend");
    }

    bool MgmGPU::is_backend_loaded() const { return data->loaded; }

    void MgmGPU::unload_backend() {
        if (!data->loaded) {
            data->log.warning("No backend loaded, nothing to unload");
            return;
        }
        data->destroy_backend(data->backend);
        data->backend = nullptr;
        data->loaded = false;

        data->log.log("Unloaded backend");
    }

    void MgmGPU::draw(const std::vector<DrawCall>& draw_list, const Settings& settings) {
        if (!is_backend_loaded())
            return;

        data->mutex.lock();

        Texture* canvas = nullptr;
        if (settings.canvas != INVALID_TEXTURE) {
            if (!data->textures.check(settings.canvas)) {
                data->log.warning("The canvas texture ", settings.canvas.id, " provided in the settings is invalid, aborting execution of draw calls");
                data->mutex.unlock();
                return;
            }
            canvas = data->textures[settings.canvas];
        }

        apply_settings(settings.backend);

        for (const auto& call : draw_list) {
            switch (call.type) {
                case DrawCall::Type::CLEAR: {
                    data->execute(data->backend, canvas);
                    data->clear(data->backend, canvas);
                    break;
                }
                case DrawCall::Type::DRAW: {
                    std::vector<Texture*> textures{};
                    for (const auto& tex : call.textures) {
                        if (tex != INVALID_TEXTURE) {
                            if (!data->textures.check(tex)) {
                                data->log.warning("Tried executing a draw call using an invalid texture ", tex.id, ", ignoring");
                                break;
                            }
                            textures.emplace_back(data->textures[tex]);
                        }
                    }
                    if (textures.size() != call.textures.size())
                        break;

                    if (!data->shaders.check(call.shader)) {
                        data->log.warning("Tried executing a draw call using an invalid shader ", call.shader.id, ", ignoring");
                        break;
                    }
                    if (!data->buffers_objects.check(call.buffers_object)) {
                        data->log.warning("Tried executing a draw call using an invalid buffers object ", call.buffers_object.id, ", ignoring");
                        break;
                    }

                    data->push_draw_call(data->backend, data->shaders[call.shader], data->buffers_objects[call.buffers_object], textures.data(), textures.size(), call.parameters);
                    break;
                }
                case DrawCall::Type::COMPUTE: {
                    break;
                }
                case DrawCall::Type::SETTINGS_CHANGE: {
                    data->execute(data->backend, canvas);
                    const auto it = call.parameters.find("settings");
                    if (it == call.parameters.end())
                        data->log.error("SETTINGS_CHANGE draw call missing \"settings\" parameter");
                    else {
                        apply_settings(std::any_cast<const GPUSettings&>(it->second));
                    }
                    break;
                }
            }
        }

        data->execute(data->backend, canvas);
        data->mutex.unlock();
    }

    GPUSettings MgmGPU::get_settings() const {
        data->mutex.lock();
        const auto settings = data->old_settings;
        data->mutex.unlock();
        return settings;
    }

    void MgmGPU::present() { data->present(data->backend); }

    MgmGPU::BufferHandle MgmGPU::create_buffer(const BufferCreateInfo& info) {
        if (!is_backend_loaded())
            return INVALID_BUFFER;

        const auto buf = data->create_buffer(data->backend, info);
        if (buf == nullptr)
            return INVALID_BUFFER;

        data->mutex.lock();
        const auto handle = static_cast<BufferHandle>(data->buffers.create(buf, info));
        data->mutex.unlock();
        return handle;
    }

    void MgmGPU::update_buffer(BufferHandle buffer, const BufferCreateInfo& info) {
        if (!is_backend_loaded())
            return;

        data->mutex.lock();
        if (!data->buffers.check(buffer)) {
            data->log.warning("Tried to update an invalid buffer handle ", buffer.id, ", ignoring");
            data->mutex.unlock();
            return;
        }
        const auto buf = data->buffers[buffer];
        data->mutex.unlock();

        if (info.type() != buf.original_info.type())
            data->log.error("Buffer type mismatch when updating buffer data");
        if (info.type_id_hash() != buf.original_info.type_id_hash())
            data->log.error("Buffer data type mismatch when updating buffer data");

        data->buffer_data(data->backend, buf.buffer, info.data(), info.size());
    }

    void MgmGPU::destroy_buffer(BufferHandle buffer) {
        if (!is_backend_loaded())
            return;
        if (buffer == INVALID_BUFFER)
            return;

        data->mutex.lock();
        if (!data->buffers.check(buffer)) {
            data->log.warning("Tried to destroy an invalid buffer handle ", buffer.id, ", ignoring");
            data->mutex.unlock();
            return;
        }
        const auto buf = data->buffers[buffer];
        data->buffers.destroy(buffer);
        data->mutex.unlock();

        data->destroy_buffer(data->backend, buf.buffer);
    }

    MgmGPU::BuffersObjectHandle MgmGPU::create_buffers_object(const std::unordered_map<std::string, BufferHandle>& buffers) {
        if (!is_backend_loaded())
            return INVALID_BUFFERS_OBJECT;

        std::vector<Buffer*> raw_buffers{};
        std::vector<std::string> buffer_names{};
        data->mutex.lock();
        for (const auto& [buf_name, buf] : buffers) {
            if (!data->buffers.check(buf)) {
                data->log.warning("Tried to use an invalid buffer ", buf.id, " to create a buffers object, ignoring");
                data->mutex.unlock();
                return INVALID_BUFFERS_OBJECT;
            }
            raw_buffers.emplace_back(data->buffers[buf].buffer);
            buffer_names.emplace_back(buf_name);
        }
        data->mutex.unlock();

        const auto obj = data->create_buffers_object(data->backend, raw_buffers.data(), buffer_names.data(), raw_buffers.size());
        if (obj == nullptr)
            return INVALID_BUFFERS_OBJECT;

        data->mutex.lock();
        const auto handle = static_cast<BuffersObjectHandle>(data->buffers_objects.create(obj));
        data->mutex.unlock();
        return handle;
    }

    void MgmGPU::destroy_buffers_object(BuffersObjectHandle buffers_object) {
        if (!is_backend_loaded())
            return;
        if (buffers_object == INVALID_BUFFERS_OBJECT)
            return;

        data->mutex.lock();
        if (!data->buffers_objects.check(buffers_object)) {
            data->log.warning("Tried to destroy an invalid buffers object handle ", buffers_object.id, ", ignoring");
            data->mutex.unlock();
            return;
        }
        const auto buf = data->buffers_objects[buffers_object];
        data->buffers_objects.destroy(buffers_object);
        data->mutex.unlock();

        data->destroy_buffers_object(data->backend, buf);
    }

    MgmGPU::ShaderHandle MgmGPU::create_shader(const MgmGPUShaderBuilder& builder) {
        if (!is_backend_loaded())
            return INVALID_SHADER;

        const auto shader = data->create_shader(data->backend, builder);
        if (shader == nullptr)
            return INVALID_SHADER;

        data->mutex.lock();
        const auto handle = static_cast<ShaderHandle>(data->shaders.create(shader));
        data->mutex.unlock();
        return handle;
    }

    void MgmGPU::destroy_shader(ShaderHandle shader) {
        if (!is_backend_loaded())
            return;
        if (shader == INVALID_SHADER)
            return;

        data->mutex.lock();
        if (!data->shaders.check(shader)) {
            data->log.warning("Tried to destroy an invalid shader handle ", shader.id, ", ignoring");
            data->mutex.unlock();
            return;
        }
        const auto sh = data->shaders[shader];
        data->shaders.destroy(shader);
        data->mutex.unlock();

        data->destroy_shader(data->backend, sh);
    }

    MgmGPU::TextureHandle MgmGPU::create_texture(const TextureCreateInfo& info) {
        if (!is_backend_loaded())
            return INVALID_TEXTURE;

        const auto texture = data->create_texture(data->backend, info);
        if (texture == nullptr)
            return INVALID_TEXTURE;

        data->mutex.lock();
        const auto handle = static_cast<TextureHandle>(data->textures.create(texture));
        data->mutex.unlock();

        return handle;
    }

    void MgmGPU::destroy_texture(TextureHandle texture) {
        if (!is_backend_loaded())
            return;
        if (texture == INVALID_TEXTURE)
            return;

        data->mutex.lock();
        if (!data->textures.check(texture)) {
            data->log.warning("Tried to destroy an invalid texture handle ", texture.id, ", ignoring");
            data->mutex.unlock();
            return;
        }
        const auto tex = data->textures[texture];
        data->textures.destroy(texture);
        data->mutex.unlock();

        data->destroy_texture(data->backend, tex);
    }

    bool MgmGPU::is_valid(const BufferHandle handle) const {
        std::unique_lock lock{data->mutex};
        return data->buffers.check(handle);
    }

    bool MgmGPU::is_valid(const BuffersObjectHandle handle) const {
        std::unique_lock lock{data->mutex};
        return data->buffers_objects.check(handle);
    }

    bool MgmGPU::is_valid(const TextureHandle handle) const {
        std::unique_lock lock{data->mutex};
        return data->textures.check(handle);
    }

    bool MgmGPU::is_valid(const ShaderHandle handle) const {
        std::unique_lock lock{data->mutex};
        return data->shaders.check(handle);
    }

    MgmGPU::~MgmGPU() {
        unload_backend();
        delete data;
    }
} // namespace mgm
