#include "systems/input.hpp"
#include "systems/notifications.hpp"
#include "file.hpp"
#include "engine.hpp"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "json.hpp"
#include "mgmwin.hpp"


namespace mgm {
    void Input::register_input_action(const std::string &name, MgmWindow::InputInterface input, const std::vector<MgmWindow::InputInterface> &modifiers, bool overwrite) {
        auto& action = input_actions[name];
        if (action.inputs.empty()) {
            if (input != MgmWindow::InputInterface::NONE) {
                action.inputs.emplace_back(input);
            }
            else if (!modifiers.empty()) {
                Logging{"Input"}.error("Action \"", name, "\" has no input, ignoring modifiers");
                return;
            }
            action.inputs.insert(action.inputs.end(), modifiers.begin(), modifiers.end());
        }
        else {
            if (overwrite) {
                action = Action{};
                action.inputs.emplace_back(input);
                action.inputs.insert(action.inputs.end(), modifiers.begin(), modifiers.end());
            }
        }
    }

    void Input::auto_register_input_action(const std::string &name) {
        auto_register_queue.insert(name);
    }

    bool Input::action_exists(const std::string& name) const {
        return input_actions.find(name) != input_actions.end();
    }

    bool Input::is_action_pressed(const std::string &name) const {
        const auto it = input_actions.find(name);
        if (it == input_actions.end()) {
            Logging{"Input"}.error("Action \"", name, "\" does not exist");
            return false;
        }
        const auto& action = it->second;
        return action.pressed;
    }
    bool Input::is_action_released(const std::string &name) const {
        const auto it = input_actions.find(name);
        if (it == input_actions.end()) {
            Logging{"Input"}.error("Action \"", name, "\" does not exist");
            return false;
        }
        const auto& action = it->second;
        return !action.pressed;
    }

    bool Input::is_action_just_pressed(const std::string &name) const {
        const auto it = input_actions.find(name);
        if (it == input_actions.end()) {
            Logging{"Input"}.error("Action \"", name, "\" does not exist");
            return false;
        }
        const auto& action = it->second;
        return action.pressed && !action.previously_pressed;
    }
    bool Input::is_action_just_released(const std::string &name) const {
        const auto it = input_actions.find(name);
        if (it == input_actions.end()) {
            Logging{"Input"}.error("Action \"", name, "\" does not exist");
            return false;
        }
        const auto& action = it->second;
        return !action.pressed && action.previously_pressed;
    }

    float Input::get_action_value(const std::string &name) const {
        const auto it = input_actions.find(name);
        if (it == input_actions.end()) {
            Logging{"Input"}.error("Action \"", name, "\" does not exist");
            return 0.0f;
        }
        const auto& action = it->second;
        if (action.inputs.size() > 1)
            Logging{"Input"}.warning("Action \"", name, "\" has modifiers, ignoring them");
        return action.value;
    }

    std::vector<Input::Callback>& Input::press_callbacks(const std::string &name) {
        const auto it = input_actions.find(name);
        if (it == input_actions.end()) {
            Logging{"Input"}.error("Action \"", name, "\" does not exist, returning __none_action__ press callback");
            return input_actions["__none_action__"].press_callbacks;
        }
        return it->second.press_callbacks;
    }
    const std::vector<Input::Callback>& Input::press_callbacks(const std::string &name) const {
        const auto it = input_actions.find(name);
        if (it == input_actions.end()) {
            Logging{"Input"}.error("Action \"", name, "\" does not exist, returning __none_action__ press callback");
            return input_actions.at("__none_action__").press_callbacks;
        }
        return it->second.press_callbacks;
    }

    std::vector<Input::Callback>& Input::release_callbacks(const std::string &name) {
        const auto it = input_actions.find(name);
        if (it == input_actions.end()) {
            Logging{"Input"}.error("Action \"", name, "\" does not exist, returning __none_action__ release callback");
            return input_actions["__none_action__"].release_callbacks;
        }
        return it->second.release_callbacks;
    }
    const std::vector<Input::Callback>& Input::release_callbacks(const std::string &name) const {
        const auto it = input_actions.find(name);
        if (it == input_actions.end()) {
            Logging{"Input"}.error("Action \"", name, "\" does not exist, returning __none_action__ release callback");
            return input_actions.at("__none_action__").release_callbacks;
        }
        return it->second.release_callbacks;
    }

#if defined(ENABLE_EDITOR)
    void Input::draw_settings_window_contents() {
        static thread_local std::string name{};

        bool create_action = ImGui::InputText("Input Action Name", &name, ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();
        if (ImGui::Button("Register") || create_action) {
            if (name.empty())
                Logging{"Input"}.error("Action name cannot be empty");
            else if (action_exists(name))
                Logging{"Input"}.error("Action \"", name, "\" already exists");
            else {
                auto_register_input_action(name);
                name.clear();
            }
        }

        for (const auto& action : input_actions) {
            ImGui::Text("%s", action.first.c_str());
            ImGui::SameLine();
            if (ImGui::Button("Delete")) {
                input_actions.erase(action.first);
                break;
            }
        }
    }
#endif

    Input::Action load(const JObject& obj) {
        Input::Action action{};
        if (obj.type() != JObject::Type::OBJECT) {
            Logging{"Input"}.error("\"inputs.json\" file invalid");
            return action;
        }
        std::vector<std::string> input_actions{};
        input_actions.emplace_back(obj["input"]);

        if (obj.has("modifiers"))
            for (const auto& [key, value] : obj["modifiers"])
                input_actions.emplace_back(value);

        for (const auto& name : input_actions) {
            const auto ii = MgmWindow::input_interface_from_name(name);

            if (ii != MgmWindow::InputInterface::NONE)
                action.inputs.emplace_back(static_cast<MgmWindow::InputInterface>(ii));
            else
                Logging{"Input"}.error("Invalid input interface name: \"", name, "\" in \"inputs.json\" file");
        }
        if (obj.has("analog"))
            action.analog = (bool)obj["analog"];
        else
            action.analog = false;

        return action;
    }

    Input::Input() {
        system_name = "Input";
        MagmaEngine engine{};
        if (!engine.file_io().exists("data://inputs.json"))
            return;

        Logging{"Input"}.log("Loading input actions from \"inputs.json\" file");

        const auto data = engine.file_io().read_text("data://inputs.json");
        const auto json = JObject{data};

        for (const auto& [key, value] : json["actions"]) {
            if (key.type() != JObject::Type::STRING || value.type() != JObject::Type::OBJECT) {
                Logging{"Input"}.error("\"inputs.json\" file invalid");
                return;
            }
            const auto action = load(value);
            input_actions[key] = action;
        }
    }

    void Input::update(float) {
        MagmaEngine engine{};
        for (auto& [name, action] : input_actions) {
            if (action.inputs.empty())
                continue;

            // Will always be true if there are no modifiers
            bool modifiers = true;
            if (!action.pressed && action.value != 0.0f)
                modifiers = false;

            if (modifiers && action.inputs.size() > 1) {
                for (size_t i = 1; i < action.inputs.size(); i++) {
                    if (engine.window().get_input_interface(action.inputs[i]) == 0.0f) {
                        modifiers = false;
                        break;
                    }
                }
            }
            action.value = engine.window().get_input_interface(action.inputs.front());

            action.previously_pressed = action.pressed;
            action.pressed = action.value != 0.0f && modifiers;

            if (!action.analog) {
                if (action.pressed && !action.previously_pressed)
                    for (const auto& callback : action.press_callbacks)
                        callback();
                else if (!action.pressed && action.previously_pressed)
                    for (const auto& callback : action.release_callbacks)
                        callback();
            }
        }

        if (!auto_register_queue.empty()) {
            const auto& name = *auto_register_queue.begin();

            engine.notifications().push("Waiting for input for while registering action \"" + name + "\"");

            for (const auto& event : engine.window().get_input_events()) {
                if (event.mode == MgmWindow::InputEvent::Mode::PRESS) {
                    const auto it = std::find(input_stack.begin(), input_stack.end(), event.input);
                    if (it == input_stack.end())
                        input_stack.emplace_back(event.input);
                }
                else if (event.mode == MgmWindow::InputEvent::Mode::RELEASE) {
                    const auto it = std::find(input_stack.begin(), input_stack.end(), event.input);
                    if (it == input_stack.begin()) {
                        const auto action = input_stack.back();
                        const auto modifiers = std::vector<MgmWindow::InputInterface>{input_stack.begin(), input_stack.end() - 1};
                        register_input_action(name, action, modifiers, true);
                        input_stack.clear();
                        auto_register_queue.erase(name);
                        break;
                    }
                }
            }
        }
    }

    Input::~Input() {
        MagmaEngine engine{};
        JObject json{};
        auto& json_actions = json["actions"];

        for (const auto& [name, action] : input_actions) {
            auto& json_action = json_actions[name];
            json_action["input"] = MgmWindow::get_input_interface_name(action.inputs.front());

            if (action.inputs.size() > 1) {
                auto& json_modifiers = json_action["modifiers"];
                for (size_t i = 1; i < action.inputs.size(); i++)
                    json_modifiers.emplace_back(MgmWindow::get_input_interface_name(action.inputs[i]));
            }

            json_action["analog"] = action.analog;
        }

        engine.file_io().write_text("data://inputs.json", json);
    }
}
