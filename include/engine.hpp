#pragma once
#include "logging.hpp"
#include <vector>


namespace mgm {
    class FileIO;
    class MgmWindow;
    class MgmGPU;
    class SystemManager;


    class MagmaEngine {
        bool initialized = false;

        static MagmaEngine* instance;

        FileIO* m_file_io = nullptr;
        MgmWindow* m_window = nullptr;
        MgmGPU* m_graphics = nullptr;
        SystemManager* m_system_manager = nullptr;

        public:
        FileIO& file_io() { return *m_file_io; }
        MgmWindow& window() { return *m_window; }
        MgmGPU& graphics() { return *m_graphics; }
        SystemManager& systems() { return *m_system_manager; }

        MagmaEngine(const MagmaEngine&) = default;
        MagmaEngine(MagmaEngine&&) = delete;
        MagmaEngine& operator=(const MagmaEngine&) = default;
        MagmaEngine& operator=(MagmaEngine&&) = delete;

        MagmaEngine(const std::vector<std::string>& args = {});

        /**
         * @brief Runs the engine
         */
        void run();

        ~MagmaEngine();
    };
}
