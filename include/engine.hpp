#pragma once
#include "logging.hpp"
#include <vector>


namespace mgm {
    class MgmWindow;
    class MgmGPU;
    class SystemManager;


    class MagmaEngine {
        bool initialized = false;

        static MagmaEngine* instance;

        public:
        MgmWindow* window = nullptr;
        MgmGPU* graphics = nullptr;

        SystemManager* system_manager = nullptr;

        SystemManager& systems() { return *system_manager; }

        MagmaEngine(const MagmaEngine&) = delete;
        MagmaEngine(MagmaEngine&&) = delete;
        MagmaEngine& operator=(const MagmaEngine&) = delete;
        MagmaEngine& operator=(MagmaEngine&&) = delete;

        MagmaEngine(const std::vector<std::string>& args = {});

        /**
         * @brief Runs the engine
         */
        void run();

        ~MagmaEngine();
    };
}
