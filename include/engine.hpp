#pragma once
#include <any>
#include <string>
#include <unordered_map>
#include <functional>


namespace mgm {
    class MgmWindow;
    class MgmGPU;

    class MagmaEngine {
        public:
        MgmWindow* window = nullptr;
        MgmGPU* graphics = nullptr;

        MagmaEngine();

        /**
         * @brief Initialize the engine
         */
        void init();

        /**
         * @brief Runs once per frame
         * 
         * @param delta Delta time for framerate (in seconds)
         */
        void tick(float delta);

        /**
         * @brief Runs once per frame, after the tic fucntion
         */
        void draw();

        /**
         * @brief Close the engine and free resources
         */
        void close();
    };
}
