#pragma once
#include "mgmlib.hpp"
#include "mgmwin.hpp"


namespace mgm {
    class MagmaEngineMainLoop {
        public:

        MgmWindow window{"MAGMA Engine", vec2i32(-1, -1), vec2u32(800, 600)};
        MgmGraphics graphics{};

        MagmaEngineMainLoop();
        
        #ifndef NDEBUG
        int tests();
        #endif

        /**
         * @brief Runs once at the beginning of the program
         * 
         */
        void init();

        /**
         * @brief Runs once per frame
         * 
         * @param delta Delta time for framerate (in seconds)
         * @return running Use to check whether the program should continue running
         */
        bool tick(float delta);

        /**
         * @brief Runs once per frame, after the tic fucntion
         * 
         * @param delta Delta time for framerate (in seconds)
         */
        void draw(float delta);

        ~MagmaEngineMainLoop();
    };
}
