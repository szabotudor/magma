#pragma once
#include <any>
#include <string>
#include <unordered_map>
#include <functional>


namespace mgm {
    class MgmWindow;
    class MgmGPU;

    class MagmaEngineMainLoop {
        public:
        MgmWindow* window = nullptr;
        MgmGPU* graphics = nullptr;

        struct MenuPage {
            using MenuEntry = std::function<void()>;
            struct MenuButton {
                std::any data{};
                float position{}, target_position{};
                bool is_menu_page = false;
            };
            std::unordered_map<std::string, MenuButton> entries;
            std::vector<std::string> entry_order{};
            std::string previous{};
        };

        std::unordered_map<std::string, MenuPage> pages{};
        std::string current_page = "main";

        bool draw_button(const std::string& name, MenuPage::MenuButton& entry);

        MagmaEngineMainLoop();

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
