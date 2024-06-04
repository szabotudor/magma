#pragma once
#include <atomic>
#include <mutex>
#include <vector>

#include "logging.hpp"


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

        bool initialized = false;

        static MagmaEngine* instance;

        ExtractedDrawData* m_imgui_draw_data = nullptr;

        FileIO* m_file_io = nullptr;
        MgmWindow* m_window = nullptr;
        MgmGPU* m_graphics = nullptr;

        SystemManager* m_system_manager = nullptr;

        std::mutex imgui_mutex;
        std::atomic_bool engine_running{};
        void render_thread_function();

        float current_dt = 0.0f;

        public:
        FileIO& file_io() { return *m_file_io; }
        MgmWindow& window() { return *m_window; }
        Input& input();
        Notifications& notifications();
        EngineManager& engine_manager();
        MgmGPU& graphics() { return *m_graphics; }
        SystemManager& systems() { return *m_system_manager; }

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
