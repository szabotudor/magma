#pragma once
#include <atomic>
#include <mutex>
#include <vector>
#include <string>


struct ImDrawData;
struct ExtractedDrawData;
struct ImFont;


namespace mgm {
    class FileIO;
    class MgmWindow;
    class Input;
    class Notifications;
    class MgmGPU;
    class SystemManager;
    class EngineManager;


    class MagmaEngine {
        friend class EngineManager;

        struct Data;
        static Data* data;
        bool initialized = false;

        std::mutex imgui_mutex{};
        std::mutex graphics_settings_mutex{};
        std::atomic_bool engine_running{};
        void render_thread_function();

        float current_dt = 0.0f;

        public:
        FileIO& file_io();
        MgmWindow& window();
        Input& input();
        Notifications& notifications();
        EngineManager& engine_manager();
        MgmGPU& graphics();
        SystemManager& systems();

        MagmaEngine(const MagmaEngine&) = delete;
        MagmaEngine(MagmaEngine&&) = delete;
        MagmaEngine& operator=(const MagmaEngine&) = delete;
        MagmaEngine& operator=(MagmaEngine&&) = delete;

        MagmaEngine(const std::vector<std::string>& args = {});

        /**
         * @brief Runs the engine
         */
        void run();

        /**
         * @brief Get the delta time at the current frame
         */
        float delta_time() const;

        ~MagmaEngine();
    };
}
