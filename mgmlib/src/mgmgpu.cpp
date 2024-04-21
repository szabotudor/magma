#include <cstring>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "backend_settings.hpp"
#include "mgmgpu.hpp"

#include "mgmwin.hpp"

#if defined(EMBED_BACKEND)
#include "backend.hpp"
#else
#include "dloader.hpp"
#endif


namespace mgm {
    template<typename T>
    class SimpleSparseSet {
        struct Container {
            T data;
            bool alive;
        };
        Container* data = nullptr;
        size_t capacity = 0;
        size_t size = 0;
        std::vector<ID_t> free_ids{};

        void alloc(size_t count) {
            if (count == 0)
                return;

            const auto new_data = (Container*)new char[count * sizeof(Container)];
            if constexpr (!std::is_trivially_constructible_v<T>) {
                for (size_t i = 0; i < size; i++) {
                    if (data[i].alive) {
                        new (&new_data[i].data) T{std::move(data[i].data)};
                        new_data[i].alive = true;
                        data[i].~Container();
                    }
                }
            }
            else if (data != nullptr && size > 0)
                memcpy(new_data, data, size * sizeof(Container));

            delete[] (char*)data;
            data = new_data;
            capacity = count;
        }

        public:
        SimpleSparseSet(size_t capacity = 0) : capacity{capacity} {
            alloc(capacity);
        }

        template<typename... Ts>
        ID_t create(Ts&&... args) {
            if (free_ids.empty()) {
                if (size == capacity)
                    alloc(capacity ? capacity * 2 : 8);
                new (&data[size].data) T{std::forward<Ts>(args)...};
                data[size].alive = true;
                return size++;
            }

            const auto id = free_ids.back();
            new (&data[id].data) T{std::forward<Ts>(args)...};
            data[id].alive = true;
            free_ids.pop_back();
            return id;
        }

        void destroy(ID_t id) {
            if (!data[id].alive)
                return;

            data[id].data.~T();
            data[id].alive = false;
            free_ids.emplace_back(id);
        }
        
        T& operator[](ID_t id) {
            return data[id].data;
        }

        const T& operator[](ID_t id) const {
            return data[id].data;
        }

        ~SimpleSparseSet() {
            if constexpr (!std::is_trivially_destructible_v<T>)
                for (size_t i = 0; i < size; i++)
                    if (data[i].alive)
                        data[i].data.~T();
            delete[] (char*)data;
        }
    };

    struct MgmGPU::Data {
        Settings old_settings{};

        struct BackendData;
        BackendData* backend = nullptr;

        using CreateBackend = BackendData*(*)(NativeWindow* window);
        using DestroyBackend = void(*)(BackendData* backend);
        using SetAttribute = bool(*)(BackendData* backend, const Settings::StateAttribute& attr, const void* data);

        using Clear = void(*)(BackendData* backend);
        using Execute = void(*)(BackendData* backend);
        using Present = void(*)(BackendData* backend);

        using CreateBuffer = struct Buffer*(*)(BackendData* backend, const BufferCreateInfo& info);
        using BufferData = void(*)(BackendData* backend, struct Buffer* buffer, const void* data, size_t size);
        using DestroyBuffer = void(*)(BackendData* backend, struct Buffer* buffer);
        using CreateBuffersObject = struct BuffersObject*(*)(BackendData* backend, Buffer** buffers, size_t count);
        using DestroyBuffersObject = void(*)(BackendData* backend, struct BuffersObject* buffers_object);
        using CreateShader = struct Shader*(*)(BackendData* backend, const ShaderCreateInfo& info);
        using DestroyShader = void(*)(BackendData* backend, struct Shader* shader);
        using CreateTexture = struct Texture*(*)(BackendData* backend, const TextureCreateInfo& info);
        using DestroyTexture = void(*)(BackendData* backend, struct Texture* texture);

        using PushDrawCall = void(*)(BackendData* backend, Shader* shader, BuffersObject* buffers_object, Texture** textures, size_t num_textures, const std::unordered_map<std::string, std::any>& parameters);

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
        std::unordered_map<Settings::StateAttribute, StateAttributeOffset> settings_offsets{};

#if !defined(EMBED_BACKEND)
        DLoader dloader{};
#endif

        Logging log{"MgmGPU"};
    };

    MgmGPU::MgmGPU(MgmGPU& gpu) {
        data = gpu.data;
        backend_settings = gpu.backend_settings;
        memset(&gpu.backend_settings, 0, sizeof(Settings));
        gpu.data = nullptr;
        gpu.window = nullptr;
    }
    MgmGPU& MgmGPU::operator=(MgmGPU& gpu) {
        data = gpu.data;
        backend_settings = gpu.backend_settings;
        memset(&gpu.backend_settings, 0, sizeof(Settings));
        gpu.data = nullptr;
        gpu.window = nullptr;
        return *this;
    }

    MgmGPU::MgmGPU(MgmWindow* window_to_connect) {
        data = new Data{};
        data->settings_offsets.emplace(Settings::StateAttribute::CLEAR, Data::StateAttributeOffset{offsetof(Settings, clear), sizeof(Settings::Clear)});
        data->settings_offsets.emplace(Settings::StateAttribute::BLENDING, Data::StateAttributeOffset{offsetof(Settings, blending), sizeof(Settings::Blending)});
        data->settings_offsets.emplace(Settings::StateAttribute::VIEWPORT, Data::StateAttributeOffset{offsetof(Settings, viewport), sizeof(Settings::Viewport)});
        data->settings_offsets.emplace(Settings::StateAttribute::SCISSOR, Data::StateAttributeOffset{offsetof(Settings, scissor), sizeof(Settings::Scissor)});

        connect_to_window(window_to_connect);
    }

    void MgmGPU::connect_to_window(MgmWindow *window_to_connect) {
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

    void MgmGPU::load_backend(const std::string &path) {
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
        data->dloader.load(path.c_str());

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

        data->log.log("Loaded backend");

        apply_settings(true);
    }

    bool MgmGPU::is_backend_loaded() const {
        return data->loaded;
    }

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

    void MgmGPU::apply_settings(bool force) {
        if (!is_backend_loaded()) return;
        if (!settings_changed) return;

        for (const auto& [attr, offset] : data->settings_offsets) {
            const auto changed = force ? true : (std::memcmp(
                reinterpret_cast<const char*>(&backend_settings) + offset.offset,
                reinterpret_cast<const char*>(&data->old_settings) + offset.offset,
                offset.size
            ) != 0);

            if (changed)
                data->set_attribute(data->backend, attr, reinterpret_cast<const char*>(&backend_settings) + offset.offset);
        }

        data->old_settings = backend_settings;
        settings_changed = false;
    }

    void MgmGPU::draw() {
        if (!is_backend_loaded()) return;
        apply_settings();

        if (draw_list.empty()) {
            data->clear(data->backend);
            data->execute(data->backend);
            return;
        }

        for (const auto& call : draw_list) {
            switch (call.type) {
                case DrawCall::Type::CLEAR: {
                    data->execute(data->backend);
                    data->clear(data->backend);
                    break;
                }
                case DrawCall::Type::DRAW: {
                    std::vector<Texture*> textures{};
                    for (const auto& tex : call.textures)
                        if (tex != INVALID_TEXTURE)
                            textures.emplace_back(data->textures[tex]);

                    data->push_draw_call(data->backend, data->shaders[call.shader], data->buffers_objects[call.buffers_object], textures.data(), textures.size(), call.parameters);
                    break;
                }
                case DrawCall::Type::COMPUTE: {
                    break;
                }
                case DrawCall::Type::SETTINGS_CHANGE: {
                    data->execute(data->backend);
                    const auto it = call.parameters.find("settings");
                    if (it == call.parameters.end())
                        data->log.error("SETTINGS_CHANGE draw call missing \"settings\" parameter");
                    else {
                        settings() = std::any_cast<Settings>(it->second);
                        apply_settings();
                    }
                    break;
                }
            }
        }

        data->execute(data->backend);
    }

    void MgmGPU::present() {
        data->present(data->backend);
    }

    MgmGPU::BufferHandle MgmGPU::create_buffer(const BufferCreateInfo &info) {
        if (!is_backend_loaded()) return INVALID_BUFFER;
        apply_settings();
        
        return data->buffers.create(data->create_buffer(data->backend, info), info);
    }

    void MgmGPU::update_buffer(BufferHandle buffer, const BufferCreateInfo &info) {
        if (!is_backend_loaded()) return;
        apply_settings();

        if (info.type() != data->buffers[buffer].original_info.type())
            data->log.error("Buffer type mismatch when updating buffer data");
        if (info.type_id_hash() != data->buffers[buffer].original_info.type_id_hash())
            data->log.error("Buffer data type mismatch when updating buffer data");

        data->buffer_data(data->backend, data->buffers[buffer].buffer, info.data(), info.size());
    }

    void MgmGPU::destroy_buffer(BufferHandle buffer) {
        if (!is_backend_loaded()) return;
        apply_settings();
        if (buffer == INVALID_BUFFER) return;

        data->destroy_buffer(data->backend, data->buffers[buffer].buffer);
        data->buffers.destroy(buffer);
    }

    MgmGPU::BuffersObjectHandle MgmGPU::create_buffers_object(const std::vector<BufferHandle> &buffers) {
        if (!is_backend_loaded()) return INVALID_BUFFERS_OBJECT;
        apply_settings();

        std::vector<Buffer*> raw_buffers{};
        for (const auto& buf : buffers)
            raw_buffers.emplace_back(data->buffers[buf].buffer);

        return data->buffers_objects.create(data->create_buffers_object(data->backend, raw_buffers.data(), raw_buffers.size()));
    }

    void MgmGPU::destroy_buffers_object(BuffersObjectHandle buffers_object) {
        if (!is_backend_loaded()) return;
        apply_settings();
        if (buffers_object == INVALID_BUFFERS_OBJECT) return;

        data->destroy_buffers_object(data->backend, data->buffers_objects[buffers_object]);
        data->buffers_objects.destroy(buffers_object);
    }

    MgmGPU::ShaderHandle MgmGPU::create_shader(const ShaderCreateInfo &info) {
        if (!is_backend_loaded()) return INVALID_SHADER;
        apply_settings();

        return data->shaders.create(data->create_shader(data->backend, info));
    }

    void MgmGPU::destroy_shader(ShaderHandle shader) {
        if (!is_backend_loaded()) return;
        apply_settings();
        if (shader == INVALID_SHADER) return;

        data->destroy_shader(data->backend, data->shaders[shader]);
        data->shaders.destroy(shader);
    }

    MgmGPU::TextureHandle MgmGPU::create_texture(const TextureCreateInfo &info) {
        if (!is_backend_loaded()) return INVALID_TEXTURE;
        apply_settings();

        return data->textures.create(data->create_texture(data->backend, info));
    }

    void MgmGPU::destroy_texture(TextureHandle texture) {
        if (!is_backend_loaded()) return;
        apply_settings();
        if (texture == INVALID_TEXTURE) return;

        data->destroy_texture(data->backend, data->textures[texture]);
        data->textures.destroy(texture);
    }

    MgmGPU::~MgmGPU() {
        unload_backend();
        delete data;
    }
} // namespace mgm
