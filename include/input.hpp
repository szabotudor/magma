#pragma once
#include "mgmwin.hpp"
#include "systems.hpp"
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "editor_windows/settings.hpp"


namespace mgm {
    class Input : System {
        public:
        enum class CallbackType {
            PRESS, RELEASE
        };
        using Callback = std::function<void()>;

        struct Action {
            std::vector<Callback> press_callbacks{};
            std::vector<Callback> release_callbacks{};

            bool analog = false;

            bool pressed = false, previously_pressed = false;
            float value = 0.0f;

            std::vector<MgmWindow::InputInterface> inputs{};
        };

        private:
        std::unordered_map<std::string, Action> input_actions{};
        std::unordered_set<std::string> auto_register_queue{};

        std::vector<MgmWindow::InputInterface> input_stack{};

        public:
        Input();

        /**
         * @brief Register an input action
         * 
         * @param name The name of the action
         * @param input The input to check for
         * @param modifiers A list of inputs that must be pressed for the action to be considered pressed
         * @param overwrite If the action already exists, overwrite it, otherwise do nothing
         */
        void register_input_action(const std::string& name, MgmWindow::InputInterface input = MgmWindow::InputInterface::NONE, const std::vector<MgmWindow::InputInterface>& modifiers = {}, bool overwrite = false);

        /**
         * @brief Wait for some key combination to be pressed, then register it as an input action
         * 
         * @param name The name of the action
         */
        void auto_register_input_action(const std::string& name);

        /**
         * @brief Check if an action exists
         * 
         * @param name The name of the action
         */
        bool action_exists(const std::string& name) const;

        /**
         * @brief Check if an action is pressed
         * 
         * @param name The name of the action
         * @return true If the action is pressed
         * @return false If the action is not pressed, or if it doesn't exist
         */
        bool is_action_pressed(const std::string& name) const;

        /**
         * @brief Check if an action is released
         * 
         * @param name The name of the action
         * @return true If the action is released
         * @return false If the action is not released, or if it doesn't exist
         */
        bool is_action_released(const std::string& name) const;

        /**
         * @brief Check if an action was just pressed
         * 
         * @param name The name of the action
         * @return true If the action was just pressed
         * @return false If the action was not just pressed, or if it doesn't exist
         */
        bool is_action_just_pressed(const std::string& name) const;

        /**
         * @brief Check if an action was just released
         * 
         * @param name The name of the action
         * @return true If the action was just released
         * @return false If the action was not just released, or if it doesn't exist
         */
        bool is_action_just_released(const std::string& name) const;

        /**
         * @brief Get the value of an action
         * 
         * @param name The name of the action
         * @return float The value of the action
         */
        float get_action_value(const std::string& name) const;

        /**
         * @brief Get the list of callbacks asociated with an action
         * 
         * @param name The name of the action
         * @return std::vector<Callback>& A reference to the list of callbacks
         */
        std::vector<Callback>& press_callbacks(const std::string& name);

        /**
         * @brief Get the list of callbacks asociated with an action
         * 
         * @param name The name of the action
         * @return const std::vector<Callback>& A const reference to the list of callbacks
         */
        const std::vector<Callback>& press_callbacks(const std::string& name) const;

        /**
         * @brief Get the list of callbacks asociated with an action
         * 
         * @param name The name of the action
         * @return std::vector<Callback>& A reference to the list of callbacks
         */
        std::vector<Callback>& release_callbacks(const std::string& name);

        /**
         * @brief Get the list of callbacks asociated with an action
         * 
         * @param name The name of the action
         * @return const std::vector<Callback>& A const reference to the list of callbacks
         */
        const std::vector<Callback>& release_callbacks(const std::string& name) const;

#if defined(ENABLE_EDITOR)
        void input_map();
        SETTINGS_SUBSECTION_DRAW_FUNC(input_map)

        virtual void in_editor_update(float delta) override { update(delta); }
#endif
        virtual void update(float) override;

        ~Input();
    };
}
