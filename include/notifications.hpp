#pragma once
#include "systems.hpp"
#include <string>
#include <unordered_map>


namespace mgm {
    class Notifications : public System {
        struct Notif {
            std::string message{};
            size_t message_hash{};
            float time{};
            vec4f color{1.0f};
        };
        std::vector<Notif> notifications{};
        std::unordered_map<size_t, size_t> notif_ids{};

        float pos = 0.0f;

        public:
        Notifications() = default;

        /**
         * @brief If the remaining time in seconds of a notification is less than this value, it will start to fade out
         */
        float start_fade = 0.5f;

        /**
         * @brief If a message exceeds this length, a warning will be displayed
         */
        size_t message_length_limit = 512;

        /**
         * @brief If true, will truncate messages that exceed the length limit
         */
        bool truncate_over_length = true;

        /**
         * @brief Push a notification to the screen (or renew an existing one with the same message)
         * 
         * @param message The message to display
         * @param color The color of the message
         * @param timeout The time in seconds before the message disappears after the last push of the same message
         */
        void push(const std::string& message, const vec4f& color = {1.0f}, float timeout = 2.0f);

#if defined(ENABLE_EDITOR)
        virtual void in_editor_update(float delta) override { update(delta); }
#endif
        void update(float delta) override;

        ~Notifications() override = default;
    };
}
