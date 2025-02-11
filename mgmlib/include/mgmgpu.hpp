#pragma once
#include "file.hpp"
#include "mgmwin.hpp"
#include "shaders.hpp"
#include "types.hpp"
#include "backend_settings.hpp"

#include <string>
#include <unordered_map>
#include <any>


namespace mgm {
    class DLoader;
    class MgmGPU {
        public:
        // Handle for a buffer, which can be used to store data on the GPU
        class BufferHandle : public ID_t{ public: using ID_t::ID_t; BufferHandle() : ID_t(ID_t::_uint(-1)) {} };

        // Handle for a buffers object, which can be used to store multiple buffers and their bindings
        class BuffersObjectHandle : public ID_t{ public: using ID_t::ID_t; BuffersObjectHandle() : ID_t(ID_t::_uint(-1)) {} };

        // Handle for a texture, which can be used to store image data on the GPU
        class TextureHandle : public ID_t{ public: using ID_t::ID_t; TextureHandle() : ID_t(ID_t::_uint(-1)) {} };

        // Handle for a shader, which can be used to execute code on the GPU
        class ShaderHandle : public ID_t{ public: using ID_t::ID_t; ShaderHandle() : ID_t(ID_t::_uint(-1)) {} };
        
        static constexpr BufferHandle INVALID_BUFFER = BufferHandle{static_cast<BufferHandle::_uint>(-1)};
        static constexpr BuffersObjectHandle INVALID_BUFFERS_OBJECT = BuffersObjectHandle{static_cast<BuffersObjectHandle::_uint>(-1)};
        static constexpr TextureHandle INVALID_TEXTURE = TextureHandle{static_cast<TextureHandle::_uint>(-1)};
        static constexpr ShaderHandle INVALID_SHADER = ShaderHandle{static_cast<ShaderHandle::_uint>(-1)};

        struct DrawCall {
            enum class Type {
                CLEAR,
                DRAW,
                COMPUTE,
                SETTINGS_CHANGE
            } type = Type::DRAW;
            ShaderHandle shader = INVALID_SHADER;
            BuffersObjectHandle buffers_object = INVALID_BUFFERS_OBJECT;
            std::vector<TextureHandle> textures{};
            std::unordered_map<std::string, std::any> parameters{};
        };

        struct Settings {
            // Backend settings
            GPUSettings backend{};

            // The render target (or INVALID_TEXTURE to draw to the window framebuffer)
            TextureHandle canvas = INVALID_TEXTURE;
        };

        private:
        struct Data;
        Data* data = nullptr;
        MgmWindow* window = nullptr;

        /**
         * @brief Apply the settings, if any changes were made.
         * 
         * @param backend_settings The settings to apply
         */
        void apply_settings(const GPUSettings& backend_settings);

        public:
        MgmGPU(const MgmGPU&) = delete;
        MgmGPU& operator=(const MgmGPU&) = delete;

        MgmGPU(MgmWindow* window = nullptr);

        /**
         * @brief Connect to a window to render to
         * 
         * @param window The window to render to
         */
        void connect_to_window(MgmWindow *window_to_connect);

        /**
         * @brief Disconnect from the window, if one is connected
         */
        void disconnect_from_window();

        /**
         * @brief Load a backend
         * (Some platforms may not allow loading libraries from shared objects(so)/dynamic link libraries(dll), in which case the default backend will be used)
         * 
         * @param path The path to the backend library file, or an empty string to use the default backend on the current platform
         */
        void load_backend(const Path &path = "");

        /**
         * @brief Check if a backend is loaded
         */
        bool is_backend_loaded() const;

        /**
         * @brief Unload the backend, if one is loaded
         */
        void unload_backend();

        /**
         * @brief Run all draw calls in the list, and present the rendered image.
         * Execution is not guaranteed to be immediate, synchronious, or in order, except for clear calls, which are regarded as separators within the same frame.
         */
        void draw(const std::vector<DrawCall>& draw_calls, const Settings& settings);

        /**
         * @brief Get the settings given when "draw" was last called
         */
        GPUSettings get_settings() const;

        /**
         * @brief Present the rendered image to the window (switch buffers, etc.)
         */
        void present();

        /**
         * @brief Create a buffer
         * 
         * @param info The buffer creation info
         * @return BufferHandle A handle to the created buffer
         */
        BufferHandle create_buffer(const BufferCreateInfo &info);

        /**
         * @brief Update a buffer with new data
         * 
         * @param buffer The buffer to update
         * @param info The new buffer creation info
         */
        void update_buffer(BufferHandle buffer, const BufferCreateInfo &info);

        /**
         * @brief Create a buffers object
         * 
         * @return BuffersObjectHandle A handle to the buffer to destroy
         */
        void destroy_buffer(BufferHandle buffer);

        /**
         * @brief Create a buffers object from a list of buffers
         * 
         * @param buffers The buffers to bind to the buffers object
         * @return BuffersObjectHandle A handle to the created buffers object
         */
        BuffersObjectHandle create_buffers_object(const std::unordered_map<std::string, BufferHandle> &buffers);

        /**
         * @brief Destroy a buffers object (This will not destroy the buffers bound to it)
         * 
         * @param buffers_object A handle to the buffers object to destroy
         */
        void destroy_buffers_object(BuffersObjectHandle buffers_object);

        /**
         * @brief Create a shader using the given info
         * 
         * @param info The shader creation info
         * @return ShaderHandle A handle to the created shader
         */
        ShaderHandle create_shader(const MgmGPUShaderBuilder &builder);

        /**
         * @brief Destroy a shader
         * 
         * @param shader A handle to the shader to destroy
         */
        void destroy_shader(ShaderHandle shader);

        /**
         * @brief Create a texture using the given info
         * 
         * @param info The texture creation info
         * @return TextureHandle A handle to the created texture
         */
        TextureHandle create_texture(const TextureCreateInfo& info);

        /**
         * @brief Destroy a texture
         * 
         * @param texture A handle to the texture to destroy
         */
        void destroy_texture(TextureHandle texture);

        /**
         * @brief Check if the given buffer handle points to a valid buffer
         * 
         * @param handle The handle to check
         * @return true If the handle is valid
         * @return false If the handle is invalid, or the object it points to has been destroyed
         */
        bool is_valid(const BufferHandle handle) const;

        /**
         * @brief Check if the given buffers object handle points to a valid buffers object
         * 
         * @param handle The handle to check
         * @return true If the handle is valid
         * @return false If the handle is invalid, or the object it points to has been destroyed
         */
        bool is_valid(const BuffersObjectHandle handle) const;

        /**
         * @brief Check if the given texture handle points to a valid texture
         * 
         * @param handle The handle to check
         * @return true If the handle is valid
         * @return false If the handle is invalid, or the object it points to has been destroyed
         */
        bool is_valid(const TextureHandle handle) const;

        /**
         * @brief Check if the given shader handle points to a valid shader
         * 
         * @param handle The handle to check
         * @return true If the handle is valid
         * @return false If the handle is invalid, or the object it points to has been destroyed
         */
        bool is_valid(const ShaderHandle handle) const;

        ~MgmGPU();
    };
}
