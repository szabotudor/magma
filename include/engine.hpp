#pragma once


namespace mgm {
    class MgmWindow;
    class MgmGPU;

    class MagmaEngineMainLoop {
        public:
        MgmWindow* window = nullptr;
        MgmGPU* graphics = nullptr;


        MagmaEngineMainLoop() = default;

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
