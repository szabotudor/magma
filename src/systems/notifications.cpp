#include "systems/notifications.hpp"
#include "helpers.hpp"
#include "imgui.h"


namespace mgm {
    size_t hash(const std::string& str) {
        return std::hash<std::string>{}(str);
    }

    void Notifications::push(const std::string &message, const vec4f& color, float timeout) {
        if (message.size() > message_length_limit) {
            Logging{"Notifications"}.warning("Message exceeds length limit: ", std::to_string(message.size()), " > ", std::to_string(message_length_limit));
            if (truncate_over_length) {
                push(message.substr(0, message_length_limit - 3) + "...");
                return;
            }
        }
        if (timeout < start_fade)
            Logging{"Notifications"}.warning("Timeout is less than start fade time: ", std::to_string(timeout), " < ", std::to_string(start_fade));

        const auto msg_hash = hash(message);
        const auto notif = notif_ids.find(msg_hash);

        if (notif == notif_ids.end()) {
            notifications.insert(notifications.begin(), Notif{message, msg_hash, timeout, color});
            for (auto& id : notif_ids)
                id.second++;
            notif_ids[msg_hash] = 0;
            pos -= ImGui::GetTextLineHeightWithSpacing();
        }
        else {
            notifications[notif->second].time = timeout;
        }
    }

    void Notifications::update(float delta) {
        ImGui::SetNextWindowPos(ImVec2{0.0f, 0.0f});
        ImGui::Begin("Notifications", nullptr,
            ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoSavedSettings
            | ImGuiWindowFlags_AlwaysAutoResize
            | ImGuiWindowFlags_NoFocusOnAppearing
            | ImGuiWindowFlags_NoNav
            | ImGuiWindowFlags_NoBackground
            | ImGuiWindowFlags_NoDocking
        );

        ImGui::SetCursorPosY(pos);

        std::vector<size_t> to_remove{};

        for (auto& notif : notifications) {
            bool using_opacity = false;
            if (notif.time < start_fade) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{notif.color.x, notif.color.y, notif.color.z, notif.color.w * notif.time / start_fade});
                using_opacity = true;
            }

            ImGui::Text("%s", notif.message.c_str());

            if (using_opacity)
                ImGui::PopStyleColor();
            notif.time -= delta;

            if (notif.time <= 0.0f)
                to_remove.push_back((size_t)(&notif - &notifications[0]));
        }

        size_t count = 0;
        for (const auto id : to_remove) {
            notif_ids.erase(notifications[id - count].message_hash);
            notifications.erase(notifications.begin() + static_cast<int64_t>(id - count++));
        }

        pos = std::lerp_with_delta(pos, 0.0f, 50.0f, delta);

        ImGui::End();
    }
}
