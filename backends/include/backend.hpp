#pragma once
#include <any>
#include <unordered_map>

#include "backend_settings.hpp"
#include "native_window.hpp"
#include "shaders.hpp"

#if !defined(WIN32)
#define EXPORT extern "C"
#else
#define EXPORT extern "C" __declspec(dllexport)
#endif


namespace mgm {
    /**
     * @brief A Buffer is a piece of memory that can be used to store data for the GPU
     */
    struct Buffer;

    /**
     * @brief A BuffersObject is a collection of Buffers that can be bound at once
     */
    struct BuffersObject;

    /**
     * @brief A Shader is a program that runs on the GPU
     */
    struct Shader;

    /**
     * @brief A Texture is an image that can be used in a shader
     */
    struct Texture;

    /**
     * @brief BackendData is a struct that contains all the data needed for the backend to function
     */
    struct BackendData;


    //============================
    // Backend graphics functions
    //============================

    /**
     * @brief Set an attribute for the backend
     *
     * @param backend The backend to set the attribute for
     * @param attr The attribute to set
     * @param data The data to set the attribute to
     * @return true if the attribute was set successfully, false otherwise
     */
    EXPORT bool set_attribute(BackendData* backend, const GPUSettings::StateAttribute& attr, const void* data);

    /**
     * @brief Clear the screen
     *
     * @param backend The backend to use
     */
    EXPORT void clear(BackendData* backend, Texture* canvas = nullptr);

    /**
     * @brief Execute all draw calls pushed this frame
     *
     * @param backend The backend to use
     * @param canvas The texture to draw to. If nullptr, will draw directly to the screen
     */
    EXPORT void execute(BackendData* backend, Texture* canvas = nullptr);

    /**
     * @brief Present the frame to the screen
     *
     * @param backend The backend to use
     */
    EXPORT void present(BackendData* backend);

    /**
     * @brief Create a buffer
     *
     * @param backend The backend to use
     * @param info The information needed to create the buffer
     * @return A pointer to the created buffer
     */
    EXPORT void* create_buffer(BackendData* backend, const BufferCreateInfo& info);

    /**
     * @brief Fill a buffer with data
     *
     * @param backend The backend to use
     * @param buffer The buffer to fill
     * @param data The data to fill the buffer with
     * @param size The size of the data
     */
    EXPORT void buffer_data(BackendData* backend, Buffer* buffer, void* data, size_t size);

    /**
     * @brief Destroy a buffer
     *
     * @param backend The backend to use
     * @param buffer The buffer to destroy
     */
    EXPORT void destroy_buffer(BackendData* backend, Buffer* buffer);

    /**
     * @brief Create a BuffersObject
     *
     * @param backend The backend to use
     * @param buffers The buffers to bind into a BuffersObject
     * @param count The number of buffers
     * @return A pointer to the created BuffersObject
     */
    EXPORT BuffersObject* create_buffers_object(BackendData* backend, Buffer** buffers, const std::string* names, size_t count);

    /**
     * @brief Destroy a BuffersObject
     *
     * @param backend The backend to use
     * @param buffers_object The BuffersObject to destroy
     */
    EXPORT void destroy_buffers_object(BackendData* backend, BuffersObject* buffers_object);

    /**
     * @brief Create a shader
     *
     * @param backend The backend to use
     * @param info The information needed to create the shader
     * @return A pointer to the created shader
     */
    EXPORT Shader* create_shader(BackendData* backend, const MgmGPUShaderBuilder& info);

    /**
     * @brief Destroy a shader
     *
     * @param backend The backend to use
     * @param shader The shader to destroy
     */
    EXPORT void destroy_shader(BackendData* backend, Shader* shader);

    /**
     * @brief Create a texture
     *
     * @param backend The backend to use
     * @param info The information needed to create the texture
     * @return A pointer to the created texture
     */
    EXPORT Texture* create_texture(BackendData* backend, const TextureCreateInfo& info);

    /**
     * @brief Destroy a texture
     *
     * @param backend The backend to use
     * @param texture The texture to destroy
     */
    EXPORT void destroy_texture(BackendData* backend, Texture* texture);

    /**
     * @brief Push a draw call to the backend
     *
     * @param backend The backend to use
     * @param shader The shader to use
     * @param buffers_object The BuffersObject to use
     * @param textures The textures to use
     * @param num_textures The number of textures
     * @param parameters The parameters to use
     */
    EXPORT void push_draw_call(BackendData* backend, Shader* shader, BuffersObject* buffers_object, Texture** textures, size_t num_textures, const std::unordered_map<std::string, std::any>& parameters);


    //====================================
    // Backend initialization and freeing
    //====================================

    /**
     * @brief Create a backend
     *
     * @param native_window The window to create the backend for
     * @return A pointer to the created backend
     */
    EXPORT BackendData* create_backend(NativeWindow* native_window);

    /**
     * @brief Destroy a backend
     *
     * @param backend The backend to destroy
     */
    EXPORT void destroy_backend(BackendData* backend);
} // namespace mgm
