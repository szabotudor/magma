#pragma once


namespace mgm {
    class MgmWindow;
    class MgmGraphics;

    class MagmaEngineMainLoop {
        public:
        MgmWindow* window = nullptr;
        MgmGraphics* graphics = nullptr;


        MagmaEngineMainLoop();

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
         * @brief Check if the engine is running
         */
        bool running();

        ~MagmaEngineMainLoop();
    };
}
