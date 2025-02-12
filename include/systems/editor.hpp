#pragma once
#include "file.hpp"
#include "systems.hpp"
#include <algorithm>
#include <type_traits>
#include <vector>


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
         * @brief Called while the window is open. Default implementation opens a resizable window with a close button. Override
         * to change the default behavior of drawing the window frame/container
         */
        virtual void draw_window();

        /**
         * @brief Called when drawing window contents. Override to add contents to your window
         */
        virtual void draw_contents() {}

        void close_window() { open = false; }

        void close_and_remove_window() {
            remove_on_close = true;
            open = false;
        }

        virtual ~EditorWindow() = default;
    };

    class Editor : public System {
        friend class EditorWindow;
        friend class Inspector;
        friend class SettingsWindow;

        void* font_id = nullptr;
        float palette_window_height = 0.0f;
        std::vector<EditorWindow*> windows;
        float time_since_last_save = 0.0f;

        bool project_initialized = false;

        std::vector<Path> recent_project_dirs{};

        std::string project_name = "";
        Path main_scene_path = "";

      public:
        bool palette_open = false;

        Editor();

      private:
        struct HoveredVectorInfo {
            std::string name{};
            float window_height = 0.0f;
        };
        std::vector<HoveredVectorInfo> hovered_vector_names{};
        size_t vector_depth = 0, max_vector_depth = 0;

      public:
        /**
         * @brief Draw a button, and open a window when said button is hovered. Remember to call "end_window_here" after you
         * finish drawing the contents
         *
         * @param name The name of the button, and the window
         * @param has_elements Whether the window has elements, or if the button should appear disabled
         * @return true If the window is open
         */
        bool begin_window_here(std::string name, bool has_elements);

        /**
         * @brief End the window opened by begin_window_here
         */
        void end_window_here();

        /**
         * @brief Check if the game is running
         *
         * @return true If the game is running in the editor
         * @return false If the editor is open and the game isn't being played
         */
        bool is_running() const;

        void draw_settings_window_contents() override;

        void update(float delta) override;

        bool draw_palette_options() override;

        static bool location_contains_project(const Path& project_path);
        static Path currently_loaded_project();
        static bool is_a_project_loaded();

        static void load_project(const Path& project_path);
        static void save_current_project();
        static void initialize_project(const Path& project_path);
        static void unload_project();

        template<typename T, typename... Ts>
            requires std::is_base_of_v<EditorWindow, T> && std::is_constructible_v<T, Ts...>
        T* add_window(bool remove_on_close = false, Ts&&... args) {
            auto window = new T{std::forward<Ts>(args)...};
            window->remove_on_close = remove_on_close;
            for (const auto& w : windows) {
                if (w->window_name == window->window_name) {
                    delete window;
                    w->open = true;
                    return dynamic_cast<T*>(w);
                }
            }
            windows.push_back(window);
            return window;
        }

        void remove_window(EditorWindow* window) {
            windows.erase(std::remove(windows.begin(), windows.end(), window), windows.end());
            delete window;
        }

        ~Editor() override;
    };
} // namespace mgm
