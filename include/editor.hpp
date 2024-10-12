#pragma once
#include "systems.hpp"
#include <algorithm>
#include <type_traits>


namespace mgm {
    class EditorWindow {
        friend class Editor;
        friend class Inspector;

        bool remove_on_close = false;

        public:
        std::string window_name = "EditorWindow";
        bool open = true;

        EditorWindow() = default;

        /**
         * @brief Called while the window is open. Default implementation opens a resizeavle window with a close button. Override to change the default behavior of drawing the window frame/container
         */
        virtual void draw_window();

        /**
         * @brief Called when drawing window contents. Override to add contents to your window
         */
        virtual void draw_contents() {};

        virtual ~EditorWindow() = default;
    };

    class Editor : public System {
        friend class EditorWindow;
        friend class Inspector;
        friend class SettingsWindow;

        void* font_id = nullptr;
        float palette_window_height = 0.0f;
        std::vector<EditorWindow*> windows;

        bool project_initialized = false;

        public:
        bool palette_open = false;

        Editor();

        void update(float delta) override;

        bool draw_palette_options() override;

        template<typename T, typename... Ts, std::enable_if_t<std::is_base_of_v<EditorWindow, T> && std::is_constructible_v<T, Ts...>, bool> = true>
        void add_window(bool remove_on_close = false, Ts&&... args) {
            auto window = new T{std::forward<Ts>(args)...};
            window->remove_on_close = remove_on_close;
            windows.push_back(window);
        }

        void remove_window(EditorWindow* window) {
            windows.erase(std::remove(windows.begin(), windows.end(), window), windows.end());
            delete window;
        }

        ~Editor() override;
    };
}
