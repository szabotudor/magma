#include <chrono>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "backend_settings.hpp"
#include "mgmgpu.hpp"

#include "mgmwin.hpp"
#include "threadpool.hpp"

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
        std::recursive_mutex mutex{};

        template<std::enable_if_t<std::is_move_constructible_v<T> || std::is_move_assignable_v<T>, bool> = true>
        void alloc(size_t count) {
            std::lock_guard lock{mutex};
            if (count == 0)
                return;

            const auto new_data = reinterpret_cast<Container*>(new char[count * sizeof(Container)]{});
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

            delete[] reinterpret_cast<char*>(data);
            data = new_data;
            capacity = count;
        }

        public:
        SimpleSparseSet(size_t set_capacity = 0) : capacity{set_capacity} {
            alloc(set_capacity);
        }

        template<typename... Ts, std::enable_if_t<std::is_constructible_v<T, Ts...>, bool> = true>
        ID_t create(Ts&&... args) {
            std::lock_guard lock{mutex};

            if (free_ids.empty()) {
                if (size == capacity)
                    alloc(capacity ? capacity * 2 : 8);
                new (&data[size].data) T{std::forward<Ts>(args)...};
                data[size].alive = true;
                return static_cast<ID_t>(size++);
            }

            const auto id = free_ids.back();
            new (&data[id].data) T{std::forward<Ts>(args)...};
            data[id].alive = true;
            free_ids.pop_back();
            return id;
        }

        void destroy(ID_t id) {
            std::lock_guard lock{mutex};

            if (!data[id].alive)
                return;

            if constexpr (!std::is_trivially_destructible_v<T>)
                data[id].data.~T();
            else if constexpr (!std::is_pointer_v<T>)
                memset(&data[id].data, 0, sizeof(T));

            data[id].alive = false;
            free_ids.emplace_back(id);
        }

        T& operator[](ID_t id) {
            std::unique_lock lock{mutex};
            return data[id].data;
        }

        const T& operator[](ID_t id) const {
            std::unique_lock lock{mutex};
            return data[id].data;
        }

        ~SimpleSparseSet() {
            std::lock_guard lock{mutex};
            if constexpr (!std::is_trivially_destructible_v<T>)
                for (size_t i = 0; i < size; i++)
                    if (data[i].alive)
                        data[i].data.~T();
            delete[] (char*)data;
        }
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

        using CreateBackend = BackendData*(*)(NativeWindow* window);
        using DestroyBackend = void(*)(BackendData* backend);
        using SetAttribute = bool(*)(BackendData* backend, const GPUSettings::StateAttribute& attr, const void* data);

        using Clear = void(*)(BackendData* backend);
        using Execute = void(*)(BackendData* backend, Texture* canvas);
        using Present = void(*)(BackendData* backend);

        using CreateBuffer = Buffer*(*)(BackendData* backend, const BufferCreateInfo& info);
        using BufferData = void(*)(BackendData* backend, Buffer* buffer, const void* data, size_t size);
        using DestroyBuffer = void(*)(BackendData* backend, Buffer* buffer);
        using CreateBuffersObject = BuffersObject*(*)(BackendData* backend, Buffer** buffers, size_t count);
        using DestroyBuffersObject = void(*)(BackendData* backend, BuffersObject* buffers_object);
        using CreateShader = Shader*(*)(BackendData* backend, const ShaderCreateInfo& info);
        using DestroyShader = void(*)(BackendData* backend, Shader* shader);
        using CreateTexture = Texture*(*)(BackendData* backend, const TextureCreateInfo& info);
        using DestroyTexture = void(*)(BackendData* backend, Texture* texture);

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
            int ref_count = 0;
        };
        struct BufferObjectInfo {
            BuffersObject* object = nullptr;
            std::vector<BufferHandle> buffers{};
        };

        template<typename T>
        struct ObjectHandleInfo {
            std::mutex mutex{};
            std::condition_variable cv{};
            T object{};
            bool good = false;
            bool busy = true;

            ObjectHandleInfo() = default;

            ObjectHandleInfo(ObjectHandleInfo&& other) : object{std::move(other.object)}, good{other.good} {
                std::unique_lock<std::mutex> lock{other.mutex};
                other.cv.wait(lock, [&other]{return !other.busy;});
                other.good = false;
                lock.unlock();
                other.cv.notify_all();
            }
            ObjectHandleInfo& operator=(ObjectHandleInfo&& other) {
                if (this == &other)
                    return *this;

                std::unique_lock<std::mutex> lock{mutex};
                std::unique_lock<std::mutex> other_lock{other.mutex};
                cv.wait(lock, [this]{return !busy;});
                other.cv.wait(other_lock, [&other]{return !other.busy;});

                object = std::move(other.object);
                good = other.good;
                other.good = false;

                lock.unlock();
                other_lock.unlock();
                cv.notify_all();
                other.cv.notify_all();

                return *this;
            }

            ObjectHandleInfo(T& t) : object{std::move(t)}, good{true} {}
            ObjectHandleInfo(const T& t) : object{t}, good{true} {}
            ObjectHandleInfo& operator=(T& t) {
                std::unique_lock<std::mutex> lock{mutex};
                cv.wait(lock, [this]{return !busy;});

                object = std::move(t);
                good = true;

                lock.unlock();
                cv.notify_all();

                return *this;
            }
            ObjectHandleInfo& operator=(const T& t) {
                std::unique_lock<std::mutex> lock{mutex};
                cv.wait(lock, [this]{return !busy;});

                object = t;
                good = true;

                lock.unlock();
                cv.notify_all();

                return *this;
            }
            ~ObjectHandleInfo() = default;
        };

        SimpleSparseSet<ObjectHandleInfo<RawBufferInfo>> buffers{};
        SimpleSparseSet<BufferObjectInfo> buffers_objects{};
        SimpleSparseSet<Shader*> shaders{};
        SimpleSparseSet<ObjectHandleInfo<Texture*>> textures{};

        struct StateAttributeOffset {
            size_t offset{};
            size_t size{};
        };
        std::unordered_map<GPUSettings::StateAttribute, StateAttributeOffset> settings_offsets{};

#if !defined(EMBED_BACKEND)
        DLoader dloader{};
#endif

        std::mutex draw_mutex{};

        ThreadPool create_destroy_objects{20};

        Logging log{"MgmGPU"};
    };

    MgmGPU::MgmGPU(MgmGPU& gpu) {
        data = gpu.data;
        gpu.data = nullptr;
        gpu.window = nullptr;
    }
    MgmGPU& MgmGPU::operator=(MgmGPU& gpu) {
        data = gpu.data;
        gpu.data = nullptr;
        gpu.window = nullptr;
        return *this;
    }

    MgmGPU::MgmGPU(MgmWindow* window_to_connect) {
        data = new Data{};
        GPUSettings backend_settings{};
        data->settings_offsets.emplace(GPUSettings::StateAttribute::CLEAR, Data::StateAttributeOffset{(size_t)&backend_settings.clear - (size_t)&backend_settings, sizeof(GPUSettings::Clear)});
        data->settings_offsets.emplace(GPUSettings::StateAttribute::BLENDING, Data::StateAttributeOffset{(size_t)&backend_settings.blending - (size_t)&backend_settings, sizeof(GPUSettings::Blending)});
        data->settings_offsets.emplace(GPUSettings::StateAttribute::VIEWPORT, Data::StateAttributeOffset{(size_t)&backend_settings.viewport - (size_t)&backend_settings, sizeof(GPUSettings::Viewport)});
        data->settings_offsets.emplace(GPUSettings::StateAttribute::SCISSOR, Data::StateAttributeOffset{(size_t)&backend_settings.scissor - (size_t)&backend_settings, sizeof(GPUSettings::Scissor)});

        connect_to_window(window_to_connect);
    }

    void MgmGPU::apply_settings(const GPUSettings& backend_settings) {
        if (!is_backend_loaded()) return;

        for (const auto& [attr, offset] : data->settings_offsets) {
            const auto changed = data->initialized ? true : (std::memcmp(
                reinterpret_cast<const char*>(&backend_settings) + offset.offset,
                reinterpret_cast<const char*>(&data->old_settings) + offset.offset,
                offset.size
            ) != 0);

            if (changed)
                data->set_attribute(data->backend, attr, reinterpret_cast<const char*>(&backend_settings) + offset.offset);
        }

        if (!data->initialized)
            data->initialized = true;

        data->old_settings = backend_settings;
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

    template<typename T, typename U>
    bool take_object(SimpleSparseSet<T>& set, const U& handle, bool wait_for_incomplete_objects = false, bool ignore_busy_state = false) {
        auto& object = set[handle];
        std::unique_lock lock{object.mutex};

        bool wait_result = true;
        if (wait_for_incomplete_objects)
            object.cv.wait(lock, [&]{ return (!object.busy && object.good) || (ignore_busy_state && object.good); });
        else
            wait_result = object.cv.wait_for(lock, std::chrono::microseconds{100}, [&]{ return (!object.busy && object.good) || (ignore_busy_state && object.good); });

        if (wait_result)
            object.busy = true;

        lock.unlock();
        object.cv.notify_all();
        return wait_result;
    }

    template<typename T, typename U>
    void release_object(SimpleSparseSet<T>& set, const U& handle) {
        auto& object = set[handle];
        std::unique_lock lock{object.mutex};
        object.busy = false;
        lock.unlock();
        object.cv.notify_all();
    }

    void MgmGPU::draw(const std::vector<DrawCall>& draw_list, const GPUSettings& backend_settings, bool wait_for_incomplete_objects) {
        if (!is_backend_loaded()) return;

        data->draw_mutex.lock();
        apply_settings(backend_settings);

        for (const auto& call : draw_list) {
            switch (call.type) {
                case DrawCall::Type::CLEAR: {
                    data->execute(data->backend, nullptr);
                    data->clear(data->backend);
                    break;
                }
                case DrawCall::Type::DRAW: {
                    std::vector<Texture*> textures{};
                    TextureHandle first_bad_texture = INVALID_TEXTURE;
                    for (const auto& tex : call.textures) {
                        if (tex != INVALID_TEXTURE) {
                            bool wait_result = take_object(data->textures, tex, wait_for_incomplete_objects);

                            if (!wait_result) {
                                first_bad_texture = tex;
                                break;
                            }
                            data->textures[tex].busy = true;
                            textures.emplace_back(data->textures[tex].object);
                        }
                    }
                    const auto release_textures = [&] {
                        for (const auto& tex : call.textures) {
                            if (tex == first_bad_texture)
                                break;
                            if (tex != INVALID_TEXTURE)
                                release_object(data->textures, tex);
                        }
                    };
                    if (first_bad_texture != INVALID_TEXTURE)
                        release_textures();

                    data->push_draw_call(data->backend, data->shaders[call.shader], data->buffers_objects[call.buffers_object].object, textures.data(), textures.size(), call.parameters);

                    release_textures();

                    break;
                }
                case DrawCall::Type::COMPUTE: {
                    break;
                }
                case DrawCall::Type::SETTINGS_CHANGE: {
                    data->execute(data->backend, nullptr);
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

        data->execute(data->backend, nullptr);
        data->draw_mutex.unlock();
    }

    GPUSettings MgmGPU::get_settings() const {
        data->draw_mutex.lock();
        const auto settings = data->old_settings;
        data->draw_mutex.unlock();
        return settings;
    }

    void MgmGPU::present() {
        data->present(data->backend);
    }

    template<typename T, typename U, typename H>
    void create_object(ThreadPool& tp, std::function<void(T& object)> create_function, U& object_set, H handle) {
        tp.push_task([&, create_function] {
            auto& object = object_set[handle];

            create_function(object.object);

            std::unique_lock lock{object.mutex};
            object.busy = false;
            object.good = true;
            lock.unlock();
            object.cv.notify_all();
        });
    }

    template<typename T, typename U, typename H>
    void destroy_object(ThreadPool& tp, std::function<void(T& object)> destroy_function, U& object_set, H handle) {
        tp.push_task([&, destroy_function] {
            auto& object = object_set[handle];
            std::unique_lock lock(object.mutex);
            object.cv.wait(lock, [&]{return !object_set[handle].busy;});
            object.busy = true;
            object.good = false;
            lock.unlock();
            object.cv.notify_all();

            destroy_function(object.object);
            object.object = {};
            object_set.destroy(handle);
        });
    }

    MgmGPU::BufferHandle MgmGPU::create_buffer(BufferCreateInfo info) {
        if (!is_backend_loaded()) return INVALID_BUFFER;

        const size_t size = info.size() * info.data_point_size();
        void* const new_data = new char[size];
        memcpy(new_data, info.data(), size);
        info.raw_data = new_data;

        const auto handle = static_cast<BufferHandle>(data->buffers.create());

        // data->draw_mutex.lock();
        // auto& buffer = data->buffers[handle];
        // buffer.object.buffer = data->create_buffer(data->backend, info);
        // delete[] static_cast<char*>(info.data());
        // buffer.object.original_info = info;
        // data->draw_mutex.unlock();

        data->create_destroy_objects.push_task([this, handle, info] {
            auto& buffer = data->buffers[handle];
            buffer.object.buffer = data->create_buffer(data->backend, info);
            delete[] static_cast<char*>(info.data());
            buffer.object.original_info = info;
            std::unique_lock lock{buffer.mutex};
            buffer.busy = false;
            buffer.good = true;
            lock.unlock();
            buffer.cv.notify_all();
        });

        return handle;
    }

    void MgmGPU::update_buffer(BufferHandle handle, BufferCreateInfo info) {
        if (!is_backend_loaded()) return;

        const size_t size = info.size() * info.data_point_size();
        void* const new_data = new char[size];
        memcpy(new_data, info.data(), size);
        info.raw_data = new_data;

        data->create_destroy_objects.push_task([this, handle, info] {
            take_object(data->buffers, handle, true, true);
            const auto& buffer = data->buffers[handle];

            if (info.type() != buffer.object.original_info.type())
                data->log.error("Buffer type mismatch when updating buffer data");
            if (info.type_id_hash() != buffer.object.original_info.type_id_hash())
                data->log.error("Buffer data type mismatch when updating buffer data");

            data->buffer_data(data->backend, buffer.object.buffer, info.data(), info.size());

            release_object(data->buffers, handle);
        });
    }

    void MgmGPU::destroy_buffer(BufferHandle handle) {
        if (!is_backend_loaded()) return;
        if (handle == INVALID_BUFFER) return;

        // data->draw_mutex.lock();
        // auto& buffer = data->buffers[handle];
        // data->destroy_buffer(data->backend, buffer.object.buffer);
        // buffer.object = {};
        // data->buffers.destroy(handle);
        // data->draw_mutex.unlock();

        data->create_destroy_objects.push_task([this, handle](){
            auto& buffer = data->buffers[handle];
            std::unique_lock<std::mutex> lock{buffer.mutex};
            buffer.cv.wait(lock, [&]{ return !buffer.busy && buffer.object.ref_count == 0; });
            buffer.busy = true;
            buffer.good = false;
            lock.unlock();
            buffer.cv.notify_all();

            data->destroy_buffer(data->backend, buffer.object.buffer);
            buffer.object = {};
            data->buffers.destroy(handle);
        });
    }

    MgmGPU::BuffersObjectHandle MgmGPU::create_buffers_object(const std::vector<BufferHandle> &buffers) {
        if (!is_backend_loaded()) return INVALID_BUFFERS_OBJECT;

        const auto handle = static_cast<BuffersObjectHandle>(data->buffers_objects.create());
        auto& buffers_object = data->buffers_objects[handle];

        std::vector<Buffer*> raw_buffers{};

        for (const auto& raw_buffer_handle : buffers) {
            auto& buffer = data->buffers[raw_buffer_handle];
            std::unique_lock lock{buffer.mutex};
            buffer.cv.wait(lock, [&] { return buffer.good; });

            buffer.busy = true;
            buffer.object.ref_count++;

            raw_buffers.emplace_back(buffer.object.buffer);
            buffers_object.buffers.emplace_back(raw_buffer_handle);

            lock.unlock();
            buffer.cv.notify_all();
        }

        buffers_object.object = data->create_buffers_object(data->backend, raw_buffers.data(), raw_buffers.size());
        return handle;
    }

    void MgmGPU::destroy_buffers_object(BuffersObjectHandle handle) {
        if (!is_backend_loaded()) return;
        if (handle == INVALID_BUFFERS_OBJECT) return;

        const auto buffers_object = data->buffers_objects[handle];

        for (auto& raw_buffer_handle : buffers_object.buffers) {
            auto& buffer = data->buffers[raw_buffer_handle];
            std::unique_lock lock{buffer.mutex};

            buffer.object.ref_count--;
            if (buffer.object.ref_count == 0)
                buffer.busy = false;

            lock.unlock();
            buffer.cv.notify_all();
        }

        data->buffers_objects.destroy(handle);

        data->destroy_buffers_object(data->backend, buffers_object.object);
    }

    MgmGPU::ShaderHandle MgmGPU::create_shader(const ShaderCreateInfo &info) {
        if (!is_backend_loaded()) return INVALID_SHADER;

        const auto shader = data->create_shader(data->backend, info);
        data->draw_mutex.lock();
        const auto handle = static_cast<ShaderHandle>(data->shaders.create(shader));
        data->draw_mutex.unlock();
        return handle;
    }

    void MgmGPU::destroy_shader(ShaderHandle shader) {
        if (!is_backend_loaded()) return;
        if (shader == INVALID_SHADER) return;

        data->draw_mutex.lock();
        const auto sh = data->shaders[shader];
        data->shaders.destroy(shader);
        data->draw_mutex.unlock();

        data->destroy_shader(data->backend, sh);
    }

    MgmGPU::TextureHandle MgmGPU::create_texture(TextureCreateInfo info) {
        if (!is_backend_loaded()) return INVALID_TEXTURE;

        size_t size = (size_t)info.size.x() * (size_t)info.size.y() * (size_t)info.channel_size_in_bytes * (size_t)info.num_channels;
        void* const new_data = new char[size];
        std::memcpy(new_data, info.data, size);
        info.data = new_data;

        const auto handle = static_cast<TextureHandle>(data->textures.create());

        data->create_destroy_objects.push_task([this, handle, info](){
            auto& texture = data->textures[handle];
            texture.object = data->create_texture(data->backend, info);
            delete[] reinterpret_cast<char*>(info.data);
            std::unique_lock lock{texture.mutex};
            texture.good = true;
            texture.busy = false;
            lock.unlock();
            texture.cv.notify_all();
        });

        return handle;
    }

    void MgmGPU::destroy_texture(TextureHandle handle) {
        if (!is_backend_loaded()) return;
        if (handle == INVALID_TEXTURE) return;

        data->create_destroy_objects.push_task([this, handle](){
            std::unique_lock<std::mutex> lock{data->textures[handle].mutex};
            data->textures[handle].cv.wait(lock, [this, &handle]{return !data->textures[handle].busy;});
            data->textures[handle].busy = true;
            data->textures[handle].good = false;
            lock.unlock();
            data->textures[handle].cv.notify_all();

            data->destroy_texture(data->backend, data->textures[handle].object);
            data->textures[handle].object = nullptr;
            data->textures.destroy(handle);
        });
    }

    MgmGPU::~MgmGPU() {
        unload_backend();
        delete data;
    }
} // namespace mgm
