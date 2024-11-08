#pragma once
#include "file.hpp"
#include "systems.hpp"
#include <vector>
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
         * @brief Called while the window is open. Default implementation opens a resizable window with a close button. Override to change the default behavior of drawing the window frame/container
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
        float time_since_last_save = 0.0f;

        bool project_initialized = false;

        std::string project_name = "";
        std::vector<Path> recent_project_dirs{};

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
         * @brief Draw a button, and open a window when said button is hovered. Remember to call "end_window_here" after you finish drawing the contents
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

        void update(float delta) override;

        bool draw_palette_options() override;

        static bool location_contains_project(const Path& project_path);
        static Path currently_loaded_project();
        static bool is_a_project_loaded();

        static void load_project(const Path& project_path);
        static void save_current_project();
        static void initialize_project(const Path& project_path);
        static void unload_project();

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