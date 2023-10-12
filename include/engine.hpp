#pragma once


namespace mgm {
    class MgmWindow;
    class MgmGraphics;

    class MagmaEngineMainLoop {
        MgmWindow* window = nullptr;
        MgmGraphics* graphics = nullptr;

        public:

        MagmaEngineMainLoop();

        /**
         * @brief Runs once per frame
         * 
         * @param delta Delta time for framerate (in seconds)
         */
        void tick(float delta);

        /**
         * @brief Runs once per frame, after the tic fucntion
         * 
         * @param delta Delta time for framerate (in seconds)
         */
        void draw(float delta);

        /**
         * @brief Check if the engine is running
         */
        bool running();

        ~MagmaEngineMainLoop();
    };
}
