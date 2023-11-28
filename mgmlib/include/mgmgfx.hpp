#pragma once
#include "mgmath.hpp"
#include "logging.hpp"

#include <cstdint>
#include <vector>


namespace mgm {
    class MgmWindow;
    class Logging;
    class DLoader;
    
    class MgmGraphics {
        struct Shader;
        struct Mesh;
        struct Texture;

        public:
        using ShaderHandle = uint32_t;
        using MeshHandle = uint32_t;
        using TextureHandle = uint32_t;
        static constexpr auto bad_shader = (ShaderHandle)-1;
        static constexpr auto bad_mesh = (MeshHandle)-1;
        static constexpr auto bad_texture = (TextureHandle)-1;

        private:
        DLoader* lib = nullptr;
        struct BackendData* data = nullptr;
        bool window_connected = false;

        Logging log;

        struct BackendFunctions {
            using __alloc_backend_data = BackendData*(*)();
            using __free_backend_data = void(*)(BackendData*& data);

            using __init_backend = void(*)(BackendData* data, struct NativeWindow* native_window);
            using __destroy_backend = void(*)(BackendData* data);

            using __viewport = void(*)(BackendData* data, const vec2i32& pos, const vec2i32& size);
            using __get_viewport = vec<2, vec2i32>(*)(BackendData* data);
            using __scissor = void(*)(BackendData* data, const vec2i32& pos, const vec2i32& size);
            using __clear_color = void(*)(BackendData* data, const vec4f& color);
            using __clear = void(*)(BackendData* data);
            using __enable_blending = void(*)(BackendData* data);
            using __disable_blending = void(*)(BackendData* data);
            using __is_blending_enabled = bool(*)(BackendData* data);
            using __swap_buffers = void(*)(BackendData* data);

            using __make_shader = Shader*(*)(BackendData* data, const void* vert_data, const void* frag_data, bool precompiled);
            using __destroy_shader = void(*)(Shader* shader);
            using __make_mesh = Mesh*(*)(const vec3f* verts, const vec3f* normals, const vec4f* colors, const vec2f* tex_coords, const uint32_t num_verts,
                const uint32_t* indices, const uint32_t num_indices);
            using __change_mesh_data = void(*)(Mesh* mesh, const vec3f* verts, const vec3f* normals, const vec4f* colors, const vec2f* tex_coords, const uint32_t num_verts);
            using __change_mesh_indices = void(*)(Mesh* mesh, const uint32_t* indices, const uint32_t num_indices);
            using __destroy_mesh = void(*)(Mesh* mesh);
            using __make_texture = Texture*(*)(const uint8_t* data, const vec2u32 size, const bool use_alpha, const bool use_mipmaps);
            using __destroy_texture = void(*)(Texture* texture);
            using __draw = void(*)(const Mesh* mesh, const Shader* shader, const Texture* const* textures, const uint32_t num_textures);

            using __find_uniform = uint32_t(*)(const Shader* shader, const char* name);

            using __uniform_f = void(*)(const Shader* shader, const uint32_t uniform_id, const float& f);
            using __uniform_d = void(*)(const Shader* shader, const uint32_t uniform_id, const double& d);
            using __uniform_u = void(*)(const Shader* shader, const uint32_t uniform_id, const uint32_t& u);
            using __uniform_i = void(*)(const Shader* shader, const uint32_t uniform_id, const int32_t& i);
            using __uniform_vec2f = void(*)(const Shader* shader, const uint32_t uniform_id, const vec2f& v);
            using __uniform_vec3f = void(*)(const Shader* shader, const uint32_t uniform_id, const vec3f& v);
            using __uniform_vec4f = void(*)(const Shader* shader, const uint32_t uniform_id, const vec4f& v);
            using __uniform_vec2d = void(*)(const Shader* shader, const uint32_t uniform_id, const vec2d& v);
            using __uniform_vec3d = void(*)(const Shader* shader, const uint32_t uniform_id, const vec3d& v);
            using __uniform_vec4d = void(*)(const Shader* shader, const uint32_t uniform_id, const vec4d& v);
            using __uniform_vec2u = void(*)(const Shader* shader, const uint32_t uniform_id, const vec2u32& v);
            using __uniform_vec3u = void(*)(const Shader* shader, const uint32_t uniform_id, const vec3u32& v);
            using __uniform_vec4u = void(*)(const Shader* shader, const uint32_t uniform_id, const vec4u32& v);
            using __uniform_vec2i = void(*)(const Shader* shader, const uint32_t uniform_id, const vec2i32& v);
            using __uniform_vec3i = void(*)(const Shader* shader, const uint32_t uniform_id, const vec3i32& v);
            using __uniform_vec4i = void(*)(const Shader* shader, const uint32_t uniform_id, const vec4i32& v);
            using __uniform_mat4f = void(*)(const Shader* shader, const uint32_t uniform_id, const mat4f& v);

            __alloc_backend_data alloc_backend_data = nullptr;
            __free_backend_data free_backend_data = nullptr;

            __init_backend init_backend = nullptr;
            __destroy_backend destroy_backend = nullptr;

            __viewport viewport = nullptr;
            __get_viewport get_viewport = nullptr;
            __scissor scissor = nullptr;
            __clear_color clear_color = nullptr;
            __clear clear = nullptr;
            __enable_blending enable_blending = nullptr;
            __enable_blending disable_blending = nullptr;
            __is_blending_enabled is_blending_enabled = nullptr;
            __swap_buffers swap_buffers = nullptr;

            __make_shader make_shader = nullptr;
            __destroy_shader destroy_shader = nullptr;
            __make_mesh make_mesh = nullptr;
            __change_mesh_data change_mesh_data = nullptr;
            __change_mesh_indices change_mesh_indices = nullptr;
            __destroy_mesh destroy_mesh = nullptr;
            __make_texture make_texture = nullptr;
            __destroy_texture destroy_texture = nullptr;
            __draw draw = nullptr;

            __find_uniform find_uniform = nullptr;

            __uniform_f uniform_f = nullptr;
            __uniform_d uniform_d = nullptr;
            __uniform_u uniform_u = nullptr;
            __uniform_i uniform_i = nullptr;
            __uniform_vec2f uniform_vec2f = nullptr;
            __uniform_vec3f uniform_vec3f = nullptr;
            __uniform_vec4f uniform_vec4f = nullptr;
            __uniform_vec2d uniform_vec2d = nullptr;
            __uniform_vec3d uniform_vec3d = nullptr;
            __uniform_vec4d uniform_vec4d = nullptr;
            __uniform_vec2u uniform_vec2u = nullptr;
            __uniform_vec3u uniform_vec3u = nullptr;
            __uniform_vec4u uniform_vec4u = nullptr;
            __uniform_vec2i uniform_vec2i = nullptr;
            __uniform_vec3i uniform_vec3i = nullptr;
            __uniform_vec4i uniform_vec4i = nullptr;
            __uniform_mat4f uniform_mat4f = nullptr;
        } funcs{};

        std::vector<Mesh*> meshes{};
        std::vector<MeshHandle> deleted_meshes{};
        std::vector<Shader*> shaders{};
        std::vector<ShaderHandle> deleted_shaders{};
        std::vector<Texture*> textures{};
        std::vector<TextureHandle> deleted_textures{};

        public:
        MgmGraphics(const MgmGraphics&) = delete;
        MgmGraphics(MgmGraphics&&) = delete;
        MgmGraphics& operator=(const MgmGraphics&) = delete;
        MgmGraphics& operator=(MgmGraphics&&) = delete;



        //========
        // WINDOW
        //========

        /**
         * @brief Construct a new Mgm Video Backend object
         * 
         * @param path The path to 
         */
        explicit MgmGraphics(const char* backend_path = nullptr);

        /**
         * @brief Check if any backend is loaded
         */
        bool is_backend_loaded();

        /**
         * @brief Load the backend library from the given path. If a window is open, it will be closed
         * 
         * @param path Path to the library
         */
        void load_backend(const char* path);

        /**
         * @brief Free the graphics backend (will close the window)
         */
        void unload_backend();

        /**
         * @brief Connect the graphics backend to a window
         * 
         * @param window The window to connect to
         */
        void connect_to_window(MgmWindow& window);

        /**
         * @brief Disconnect the window the backend it connected to (THIS WILL UNLOAD THE BACKEND)
         */
        void disconnect_from_window();

        ~MgmGraphics();



        //=================
        // DYNAMIC BACKEND
        //=================

        inline void viewport(const vec2i32& pos, const vec2i32& size) {
            funcs.viewport(data, pos, size);
        }

        inline auto get_viewport() {
            return funcs.get_viewport(data);
        }

        inline void scissor(const vec2i32& pos, const vec2i32& size) {
            funcs.scissor(data, pos, size);
        }

        /**
         * @brief Set the color to clear the screen to
         * 
         * @param color The color to set
         */
        inline void clear_color(const vec4f& color) {
            funcs.clear_color(data, color);
        }

        /**
         * @brief Clear the screen to the selected solid color
         */
        inline void clear() {
            funcs.clear(data);
        }

        void enable_blending() { funcs.enable_blending(data); }
        void disable_blending() { funcs.disable_blending(data); }
        bool is_blending_enabled() const { return funcs.is_blending_enabled(data); }

        /**
         * @brief Swap window buffers (present buffer)
         */
        inline void swap_buffers() {
            funcs.swap_buffers(data);
        }

        /**
         * @brief Compile shader source into a shader
         * 
         * @param vert Source for vertex shader
         * @param frag Source for fragment shader
         * @return A handle to the shader
         */
        ShaderHandle make_shader_from_source(const char* vert, const char* frag);

        /**
         * @brief Destroy a shader
         * 
         * @param shader handle to the shader
         */
        void destroy_shader(const ShaderHandle shader);

        /**
         * @brief Create a mesh from raw data
         * 
         * @param verts Array of vertices
         * @param normals Array of normals or nullptr
         * @param colors Array of vertex colors or nullptr
         * @param tex_coords Array of vertex texture coordinates or nullptr
         * @param num_verts Number of points (point is collection of verts, normals, colors, tex_coords)
         * @param indices Array of indices, or nullptr
         * @param num_indices Number of indices in the array
         * @return A handle to the shader
         */
        MeshHandle make_mesh(const vec3f* verts, const vec3f* normals = nullptr,
                            const vec4f* colors = nullptr, const vec2f* tex_coords = nullptr,
                            const uint32_t num_verts = 0,
                            const uint32_t* indices = nullptr,
                            const uint32_t num_indices = 0);

        /**
         * @brief Change the vertex data of the mesh
         * 
         * @param mesh The mesh to manipulate
         * @param verts Array of vertices
         * @param normals Array of normals or nullptr
         * @param colors Array of vertex colors or nullptr
         * @param tex_coords Array of vertex texture coordinates or nullptr
         * @param num_verts Number of points (point is collection of verts, normals, colors, tex_coords)
         */
        void change_mesh_data(const MeshHandle mesh, const vec3f* verts, const vec3f* normals = nullptr,
                            const vec4f* colors = nullptr, const vec2f* tex_coords = nullptr,
                            const uint32_t num_verts = 0);

        /**
         * @brief Change the indices (vertex draw order) of a mesh
         * 
         * @param mesh The mesh to manipulate
         * @param indices Array of indices, or nullptr
         * @param num_indices Number of indices in the array
         */
        void change_mesh_indices(const MeshHandle mesh, const uint32_t* indices = nullptr, const uint32_t num_indices = 0);

        /**
         * @brief Destroy a mesh
         * 
         * @param mesh A handle to the mesh to destroy
         */
        void destroy_mesh(const MeshHandle mesh);

        /**
         * @brief Create a texture from the given data
         * 
         * @param data The RGB data to load the texture as (each R, G and B value is a uint8_t)
         * @param size The size (resolution) of the texture
         * @return A handle to the texture
         */
        TextureHandle make_texture(const uint8_t* data, const vec2u32 size, const bool use_alpha = false, const bool use_mipmaps = true);

        /**
         * @brief Destroy a texture
         * 
         * @param texture A handle to the texture to destroy
         */
        void destroy_texture(const TextureHandle texture);

        /**
         * @brief Draw a mesh, using a shader
         * 
         * @param mesh Mesh to draw
         * @param shader The shader to use
         */
        void draw(const MeshHandle mesh, const ShaderHandle shader, const std::vector<TextureHandle>& texture_handles = {}) const {
            std::vector<const Texture*> raw_textures{};
            raw_textures.reserve(texture_handles.size());
            for (const auto& t : texture_handles)
                raw_textures.emplace_back(textures[t]);

            funcs.draw(meshes[mesh], shaders[shader], raw_textures.data(), raw_textures.size());
        }

        uint32_t find_uniform(const ShaderHandle shader, const char* name) {
            return funcs.find_uniform(shaders[shader], name);
        }

        template<typename T>
        void uniform_data(const ShaderHandle shader, const uint32_t uniform_id, const T& value);

        template<typename T>
        void uniform_data(const ShaderHandle shader, const char* uniform_name, const T& value) {
            const auto uniform_id = find_uniform(shader, uniform_name);
            uniform_data(shader, uniform_id, value);
        }
    };

    template<typename T>
    void MgmGraphics::uniform_data(const ShaderHandle shader, const uint32_t uniform_id, const T &value) {
        log.error("Uniform type not supported");
    }
}
