#pragma once
#include "logging.hpp"
#include "mgmath.hpp"
#include <functional>
#include <vector>
#include <algorithm>


namespace mgm {
    class MgmWindow {
        public:
        enum class Mode {
            NORMAL,
            BORDERLESS,
            FULLSCREEN
        };

        enum class InputInterface {
            NONE,
            Key_A, Key_B, Key_C, Key_D, Key_E, Key_F, Key_G, Key_H, Key_I, Key_J, Key_K, Key_L, Key_M,
            Key_N, Key_O, Key_P, Key_Q, Key_R, Key_S, Key_T, Key_U, Key_V, Key_W, Key_X, Key_Y, Key_Z,
            Key_0, Key_1, Key_2, Key_3, Key_4, Key_5, Key_6, Key_7, Key_8, Key_9,
            Key_META, Key_CAPSLOCK, Key_NUMLOCK, Key_SCROLLLOCK,
            Key_SPACE, Key_ENTER, Key_TAB, Key_SHIFT, Key_CTRL, Key_ALT, Key_ESC, Key_BACKSPACE,
            Key_DELETE, Key_INSERT, Key_HOME, Key_END, Key_PAGEUP, Key_PAGEDOWN,
            Key_ARROW_UP, Key_ARROW_DOWN, Key_ARROW_LEFT, Key_ARROW_RIGHT,
            Key_F1, Key_F2, Key_F3, Key_F4, Key_F5, Key_F6, Key_F7, Key_F8, Key_F9, Key_F10, Key_F11, Key_F12,
            Key_PLUS, Key_MINUS, Key_ASTERISK, Key_EQUAL, Key_COMMA, Key_PERIOD,
            Key_COLON, Key_SEMICOLON, Key_APOSTROPHE, Key_QUOTE, Key_OPEN_BRACKET, Key_CLOSE_BRACKET, Key_OPEN_CURLY_BRACKET, Key_CLOSE_CURLY_BRACKET,
            Key_BACKSLASH, Key_FORWARD_SLASH, Key_QUESTION_MARK, Key_EXCLAMATION_MARK,
            Key_AT, Key_HASH, Key_DOLLAR, Key_PERCENT, Key_CARET, Key_LESS, Key_GREATER, Key_AMPERSAND,
            Key_OPEN_PARENTHESIS, Key_CLOSE_PARENTHESIS, Key_UNDERSCORE, Key_GRAVE,
            Key_TILDE, Key_VERTICAL_LINE,

            Mouse_LEFT, Mouse_RIGHT, Mouse_MIDDLE, Mouse_SCROLL_UP, Mouse_SCROLL_DOWN,
            Mouse_POS_X, Mouse_POS_Y,

            _NUM_INPUT_INTERFACES
        };
        static inline const std::vector<std::string> input_interface_names {
            "NONE",
            "Key_A", "Key_B", "Key_C", "Key_D", "Key_E", "Key_F", "Key_G", "Key_H", "Key_I", "Key_J", "Key_K", "Key_L", "Key_M",
            "Key_N", "Key_O", "Key_P", "Key_Q", "Key_R", "Key_S", "Key_T", "Key_U", "Key_V", "Key_W", "Key_X", "Key_Y", "Key_Z",
            "Key_0", "Key_1", "Key_2", "Key_3", "Key_4", "Key_5", "Key_6", "Key_7", "Key_8", "Key_9",
            "Key_META", "Key_CAPSLOCK", "Key_NUMLOCK", "Key_SCROLLLOCK",
            "Key_SPACE", "Key_ENTER", "Key_TAB", "Key_SHIFT", "Key_CTRL", "Key_ALT", "Key_ESC", "Key_BACKSPACE",
            "Key_DELETE", "Key_INSERT", "Key_HOME", "Key_END", "Key_PAGEUP", "Key_PAGEDOWN",
            "Key_ARROW_UP", "Key_ARROW_DOWN", "Key_ARROW_LEFT", "Key_ARROW_RIGHT",
            "Key_F1", "Key_F2", "Key_F3", "Key_F4", "Key_F5", "Key_F6", "Key_F7", "Key_F8", "Key_F9", "Key_F10", "Key_F11", "Key_F12",
            "Key_PLUS", "Key_MINUS", "Key_ASTERISK", "Key_EQUAL", "Key_COMMA", "Key_PERIOD",
            "Key_COLON", "Key_SEMICOLON", "Key_APOSTROPHE", "Key_QUOTE", "Key_OPEN_BRACKET", "Key_CLOSE_BRACKET", "Key_OPEN_CURLY_BRACKET", "Key_CLOSE_CURLY_BRACKET",
            "Key_BACKSLASH", "Key_FORWARD_SLASH", "Key_QUESTION_MARK", "Key_EXCLAMATION_MARK",
            "Key_AT", "Key_HASH", "Key_DOLLAR", "Key_PERCENT", "Key_CARET", "Key_LESS", "Key_GREATER", "Key_AMPERSAND",
            "Key_OPEN_PARENTHESIS", "Key_CLOSE_PARENTHESIS", "Key_UNDERSCORE", "Key_GRAVE",
            "Key_TILDE", "Key_VERTICAL_LINE",

            "Mouse_LEFT", "Mouse_RIGHT", "Mouse_MIDDLE", "Mouse_SCROLL_UP", "Mouse_SCROLL_DOWN",
            "Mouse_POS_X", "Mouse_POS_Y",

            "_NUM_INPUT_INTERFACES"
        };
        static InputInterface input_interface_from_name(const std::string& name) {
            const auto it = std::find(input_interface_names.begin(), input_interface_names.end(), name);
            if (it != input_interface_names.end()) {
                const size_t index = (size_t)(it - input_interface_names.begin());
                if (index < (size_t)InputInterface::_NUM_INPUT_INTERFACES)
                    return (InputInterface)index;
            }
            return InputInterface::NONE;
        }
        static std::string get_input_interface_name(const InputInterface ii) {
            if (ii <= InputInterface::_NUM_INPUT_INTERFACES)
                return input_interface_names[(size_t)ii];
            return "NONE";
        }

        float* input_interfaces = nullptr;

        /**
         * @brief Get the value of an input interface
         * 
         * @param ii The input interface
         * @return float& A reference to the value
         */
        float& get_input_interface(const InputInterface ii) {
            return input_interfaces[(size_t)ii];
        }

        /**
         * @brief Get the value of an input interface
         * 
         * @param ii The input interface
         * @return const float& A reference to the value
         */
        const float& get_input_interface(const InputInterface ii) const {
            return input_interfaces[(size_t)ii];
        }

        /**
         * @brief Get the value of an input interface at the previous update (previous frame)
         * 
         * @param ii The input interface
         * @return float& A reference to the value
         */
        float& get_input_interface_last_update(const InputInterface ii) {
            return input_interfaces[(size_t)ii + (size_t)InputInterface::_NUM_INPUT_INTERFACES];
        }

        /**
         * @brief Get the value of an input interface at the previous update (previous frame)
         * 
         * @param ii The input interface
         * @return const float& A reference to the value
         */
        const float& get_input_interface_last_update(const InputInterface ii) const {
            return input_interfaces[(size_t)ii + (size_t)InputInterface::_NUM_INPUT_INTERFACES];
        }

        /**
         * @brief Get the delta of an input interface (difference between the current and previous frame)
         * 
         * @param ii The input interface
         * @return float The delta
         */
        float get_input_interface_delta(const InputInterface ii) const {
            return get_input_interface(ii) - get_input_interface_last_update(ii);
        }

        struct InputEvent {
            enum class Mode { NONE, PRESS, RELEASE, OTHER };
            enum class From { NONE, KEYBOARD, MOUSE };

            InputInterface input{};
            float value{};
            Mode mode{};
            From from{};
        };

        private:
        std::vector<InputEvent> input_events_since_last_update{};
        std::string text_input_since_last_update{};

        std::vector<std::vector<std::function<void(InputEvent)>>> callbacks{};

        public:
        /**
         * @brief Get the input events since the last update
         * 
         * @return const auto& The input events
         */
        const auto& get_input_events() { return input_events_since_last_update; }

        /**
         * @brief Get the text input since the last update
         * 
         * @return const auto& The text input
         */
        const auto& get_text_input() { return text_input_since_last_update; }

        std::vector<std::function<void(InputEvent)>>& input_callbacks(const InputInterface ii) {
            return callbacks[(size_t)ii];
        }
        const std::vector<std::function<void(InputEvent)>>& input_callbacks(const InputInterface ii) const {
            return callbacks[(size_t)ii];
        }

        private:
        struct NativeWindow* data = nullptr;
        friend struct NativeWindow;
        Mode window_mode = Mode::NORMAL, nonfullscreen_window_mode = Mode::NORMAL;
        vec2u32 window_size{}, min_size{}, max_size{};
        vec2i32 window_pos{};
        bool _should_close = false, _is_open = false;
        uint32_t _allow_resize = 0, _allow_close = 1, _allow_maximize = 1, _allow_minimize = 1;
        
        Logging log;

        public:
        MgmWindow(const MgmWindow&) = delete;
        MgmWindow(MgmWindow&&) = delete;
        MgmWindow& operator=(const MgmWindow&) = delete;
        MgmWindow& operator=(MgmWindow&&) = delete;

        NativeWindow* get_native_window() { return data; }

        MgmWindow(const char* name = "Window", vec2u32 size = vec2u32(800, 600), Mode mode = Mode::NORMAL, vec2i32 pos = vec2i32(-1, -1)):
        log{(std::string("Window \"") + name + '\"').c_str()} {
            input_interfaces = new float[(size_t)InputInterface::_NUM_INPUT_INTERFACES * 2]{};
            callbacks.resize((size_t)InputInterface::_NUM_INPUT_INTERFACES);
            open(name, size, mode, pos);
        }

        /**
         * @brief Open the window if it's closed, or reopen it with the new information if it's already opened
         */
        void open(const char* name = "Window", vec2u32 size = vec2u32(800, 600), Mode mode = Mode::NORMAL, vec2i32 pos = vec2i32(-1, -1));

        /**
         * @brief Set the window mode
         * 
         * @param mode NONE, BORDERLESS, FULLSCREEN
         */
        void set_mode(const Mode mode);

        /**
         * @brief Get the window mode
         */
        inline Mode get_mode() const { return window_mode; }

        /**
         * @brief Allow or block resizing the window
         * 
         * @param allow True to allow, False to block
         */
        void set_allow_resize(bool allow);

        /**
         * @brief Check if resizing the window is allowed
         */
        bool get_allow_resize() const { return _allow_resize; }

        /**
         * @brief Allow or block closing the window
         * 
         * @param allow True to allow, False to block
         */
        void set_allow_close(bool allow);

        /**
         * @brief Check if closing the window is allowed
         */
        bool get_allow_close() const { return _allow_close; }

        /**
         * @brief Allow or block maximizing windows
         *
         * @param allow True to allow, False to block
         */
        void set_allow_maximize(const bool allow);

        /**
         * @brief Check if maximizing the window is allowed
         */
        bool get_allow_maximize() const { return _allow_maximize; }

        /**
         * @brief Allow or block minimizing windows
         *
         * @param allow True to allow, False to block
         */
        void set_allow_minimize(const bool allow);

        /**
         * @brief Check if minimizing the window is allowed
         */
        bool get_allow_minimize() const { return _allow_maximize; }

        /**
         * @brief Get the size of the window
         */
        vec2u32 get_size() const { return window_size; }

        /**
         * @brief Set the size of the window
         * 
         * @param size The new size
         */
        void set_size(vec2u32 size);

        /**
         * @brief Set the position of the window
         * 
         * @param pos The new position
         */
        void set_position(vec2i32 pos);

        /**
         * @brief Check if the window should close
         */
        bool should_close() const { return _should_close; }

        /**
         * @brief Ignore the close event
         */
        void ignore_close() { _should_close = false; }

        /**
         * @brief Make the window close the next time it updates
         */
        void set_should_close_next_update() { _should_close = true; }

        /**
         * @brief Force close the window
         */
        void close();

        /**
         * @brief Flush and loop through all events and update the window
         */
        void update();

        /**
         * @brief Check if the window is still open
         */
        inline bool is_open() const { return _is_open; }

        ~MgmWindow();
    };
}
