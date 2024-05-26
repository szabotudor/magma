#pragma once
#include "mgmwin.hpp"
#include "systems.hpp"
#include <string>
#include <unordered_map>
#include <unordered_set>


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

        public:
        Input() { system_name = "Input"; }

        void register_input_action(const std::string& name, MgmWindow::InputInterface input, const std::vector<MgmWindow::InputInterface>& modifiers = {});

        void auto_register_input_action(const std::string& name);

        bool is_action_pressed(const std::string& name) const;
        bool is_action_released(const std::string& name) const;
        bool is_action_just_pressed(const std::string& name) const;
        bool is_action_just_released(const std::string& name) const;

        float get_action_value(const std::string& name) const;

        std::vector<Callback>& press_callbacks(const std::string& name);
        const std::vector<Callback>& press_callbacks(const std::string& name) const;
        std::vector<Callback>& release_callbacks(const std::string& name);
        const std::vector<Callback>& release_callbacks(const std::string& name) const;


        virtual void on_begin_play() override;
        virtual void update(float) override;
        virtual void on_end_play() override;

        ~Input() = default;
    };
}
