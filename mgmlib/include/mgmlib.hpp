#pragma once
#include "mgmath.hpp"

#include <cstdint>
#include <vector>


namespace mgm {
    class MgmWindow;
    class Logging;
    class DLoader;
    
    class MgmGraphics {
        struct Shader;
        struct Mesh;

        public:
        using ShaderHandle = uint32_t;
        using MeshHandle = uint32_t;
        static constexpr auto bad_shader = (ShaderHandle)-1;
        static constexpr auto bad_mesh = (MeshHandle)-1;

        private:
        DLoader* lib = nullptr;
        struct BackendData* data = nullptr;
        bool window_connected = false;

        Logging* log = nullptr;

        struct BackendFunctions {
            using __alloc_backend_data = BackendData*(*)();
            using __free_backend_data = void(*)(BackendData*& data);

            using __init_backend = void(*)(BackendData* data, struct NativeWindow* native_window);
            using __destroy_backend = void(*)(BackendData* data);

            using __viewport = void(*)(BackendData* data, const vec2i32& pos, const vec2i32& size);
            using __clear_color = void(*)(BackendData* data, const vec4f& color);
            using __clear = void(*)(BackendData* data);
            using __swap_buffers = void(*)(BackendData* data);

            using __make_shader = Shader*(*)(const void* vert_data, const void* frag_data, bool precompiled);
            using __destroy_shader = void(*)(Shader* shader);
            using __make_mesh = Mesh*(*)(const vec3f* verts, const vec3f* normals, const vec4f* colors, const vec2f* tex_coords, const uint32_t num_verts,
                const uint32_t* indices, const uint32_t num_indices);
            using __destroy_mesh = void(*)(Mesh* mesh);
            using __draw = void(*)(const Mesh* mesh, const Shader* shader);

            __alloc_backend_data alloc_backend_data = nullptr;
            __free_backend_data free_backend_data = nullptr;

            __init_backend init_backend = nullptr;
            __destroy_backend destroy_backend = nullptr;

            __viewport viewport = nullptr;
            __clear_color clear_color = nullptr;
            __clear clear = nullptr;
            __swap_buffers swap_buffers = nullptr;

            __make_shader make_shader = nullptr;
            __destroy_shader destroy_shader = nullptr;
            __make_mesh make_mesh = nullptr;
            __destroy_mesh destroy_mesh = nullptr;
            __draw draw = nullptr;
        } funcs{};

        std::vector<Mesh*> meshes{};
        std::vector<MeshHandle> deleted_meshes{};
        std::vector<Shader*> shaders{};
        std::vector<ShaderHandle> deleted_shaders{};

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
        void disconnect_window();

        ~MgmGraphics();



        //=================
        // DYNAMIC BACKEND
        //=================

        inline void viewport(const vec2i32& pos, const vec2i32& size) {
            funcs.viewport(data, pos, size);
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
        MeshHandle make_mesh(const vec3f* verts, const vec3f* normals, const vec4f* colors, const vec2f* tex_coords, const uint32_t num_verts,
                            const uint32_t* indices, const uint32_t num_indices);

        /**
         * @brief Destroy a mesh
         * 
         * @param mesh A handle to the mesh to destroy
         */
        void destroy_mesh(const MeshHandle mesh);

        /**
         * @brief Draw a mesh, using a shader
         * 
         * @param mesh Mesh to draw
         * @param shader The shader to use
         */
        void draw(const MeshHandle mesh, const ShaderHandle shader) const {
            funcs.draw(meshes[mesh], shaders[shader]);
        }
    };
}
