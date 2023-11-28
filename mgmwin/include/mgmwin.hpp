#pragma once
#include "logging.hpp"
#include "mgmath.hpp"


namespace mgm {
    class MgmWindow {
        public:
        enum class Mode {
            NORMAL,
            BORDERLESS,
            FULLSCREEN
        };

        private:
        struct NativeWindow* data = nullptr;
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

        MgmWindow(const char* name = "Window", vec2i32 pos = vec2i32(-1, -1), vec2u32 size = vec2u32(800, 600), Mode mode = Mode::NORMAL);

        /**
         * @brief Open the window if it's closed, or reopen it with the new information if it's already opened
         */
        void open(const char* name = "Window", vec2i32 pos = vec2i32(-1, -1), vec2u32 size = vec2u32(800, 600), Mode mode = Mode::NORMAL);

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
