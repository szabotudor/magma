#pragma once
#include "mgmath.hpp"

#include <cstdint>


namespace mgm {
    class MgmWindow;
    class Logging;
    class DLoader;
    
    class MgmGraphics {
        struct DShader;
        struct DMesh;

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

            using __make_shader = DShader*(*)(const void* vert_data, const void* frag_data, bool precompiled);
            using __destroy_shader = void(*)(DShader* shader);

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
        } funcs{};

        public:
        class Shader {
            DShader* shader = nullptr;

            struct BackendFunctions {
                MgmGraphics::BackendFunctions::__destroy_shader destroy_shader = nullptr;
            } funcs;
            
            public:
            Shader(const Shader&) = delete;
            Shader(Shader&&);
            Shader& operator=(const Shader&) = delete;
            Shader& operator=(Shader&&);

            Shader(DShader* native_shader, MgmGraphics& graphics);

            DShader* get_native_shader();

            ~Shader();
        };

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
         * @return The shader
         */
        Shader make_shader_from_source(const char* vert, const char* frag);
    };
}
