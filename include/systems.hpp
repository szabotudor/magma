#pragma once
#include "logging.hpp"
#include <mutex>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>


namespace mgm {
    class SettingsWindow;
    class SystemManager;

    class System {
        friend class SystemManager;

      public:
        std::string system_name = "System";

        System() = default;

        /**
         * @brief Called when the game starts (DOES NOT CREATE THE SYSTEM, CAN BE CALLED MULTIPLE TIMES)
         */
        virtual void on_begin_play() {}

        /**
         * @brief Called once per frame
         *
         * @param delta Delta time for framerate (in seconds)
         */
        virtual void update(float delta) { (void)delta; /* Unused */ }

        /**
         * @brief Called once per draw frame, also while in the editor
         */
        virtual void graphics_update() { /* Unused */ }

#if defined(ENABLE_EDITOR)
        /**
         * @brief Called to draw the contents of the ImGui window for system settings
         */
        virtual void draw_settings_window_contents() { /* Unused */ }
        bool should_appear_in_settings_window = false;

        /**
         * @brief Called once per frame, only only while the editor is open
         *
         * @param delta Delta time for framerate (in seconds)
         */
        virtual void in_editor_update(float delta) { (void)delta; /* Unused */ }

        /**
         * @brief Called when the palette is open, to inspect options for the system
         * @return True If the palette should be closed (an option was selected)
         */
        virtual bool draw_palette_options() { return false; /* Unused */ }
#endif

        /**
         * @brief (Usually unused) Called once per frame, possibly on a different thread
         */
        virtual void draw() {}

        /**
         * @brief Called when the game ends (DOES NOT DESTROY THE SYSTEM, PLAY MIGHT BEGIN AGAIN)
         */
        virtual void on_end_play() {}

        virtual ~System() = default;
    };

    class SystemManager {
      public:
        std::unordered_map<size_t, System*> systems{};
        std::unordered_map<size_t, size_t> replacements{};
        std::recursive_mutex mutex{};

        SystemManager() = default;

        /**
         * @brief Create a new system
         *
         * @tparam T The type of the system
         * @tparam Ts Argument types to pass to the constructor
         * @param args Arguments to pass to the constructor
         * @return T& A reference to the new system
         */
        template<typename T, typename... Ts>
            requires std::is_constructible_v<T, Ts...> && std::is_base_of_v<System, T>
        T& create(Ts&&... args) {
            std::unique_lock lock{mutex};
            auto id = typeid(T).hash_code();
            const auto it = systems.find(id);
            if (it != systems.end()) {
                Logging{"SystemManager"}.warning("System already exists");
                return *reinterpret_cast<T*>(system);
            }
            auto& system = systems[id];
            system = reinterpret_cast<System*>(new T{std::forward<Ts>(args)...});
            T& sys = *reinterpret_cast<T*>(system);
            sys.should_appear_in_settings_window = std::is_same_v<decltype(&T::draw_settings_window_contents), void (T::*)()>;
            if (system->system_name.empty())
                system->system_name = typeid(T).name();
            return sys;
        }

        /**
         * @brief Get a reference to a system
         *
         * @tparam T The type of the system to get
         * @return T& A reference to the system
         */
        template<typename T>
        T& get() {
            const auto sys = try_get<T>();
            if (sys == nullptr)
                return create<T>();
            return *sys;
        }

        /**
         * @brief Get a const reference to a system
         *
         * @tparam T The type of the system to get
         * @return const T& A const reference to the system
         */
        template<typename T>
        const T& get() const {
            const auto sys = try_get<T>();
            if (sys == nullptr)
                throw std::runtime_error{"System does not exist"};
            return *sys;
        }

        /**
         * @brief Get a pointer to the given system if it exists
         *
         * @tparam T The type of the system to get
         * @return T* A pointer to the system, or nullptr if the system doesn't exist
         */
        template<typename T>
        T* try_get() {
            auto id = typeid(T).hash_code();
            auto it = systems.find(id);
            if (it == systems.end()) {
                const auto rit = replacements.find(id);
                if (rit == replacements.end())
                    return nullptr;

                it = systems.find(rit->second);
                if (it == systems.end())
                    return nullptr;
            }
            return reinterpret_cast<T*>(it->second);
        }

        /**
         * @brief Get a const pointer to the given system if it exists
         *
         * @tparam T The type of the system to get
         * @return T* A const pointer to the system, or nullptr if the system doesn't exist
         */
        template<typename T>
        const T* try_get() const {
            auto id = typeid(T).hash_code();
            const auto it = systems.find(id);
            if (it == systems.end()) {
                return nullptr;
            }
            return reinterpret_cast<T*>(it->second);
        }

        /**
         * @brief Destroy a system
         *
         * @tparam T The type of the system to destroy
         */
        template<typename T>
        void destroy() {
            std::unique_lock lock{mutex};
            auto id = typeid(T).hash_code();
            const auto it = systems.find(id);
            if (it == systems.end()) {
                Logging{"SystemManager"}.error("System does not exist!");
                return;
            }
            T& sys = *reinterpret_cast<T*>(it->second);
            sys.on_end_play();
            delete it->second;
            systems.erase(it);
        }

        /**
         * @brief Replace an existing system with a different one that inherits from the original
         *
         * @tparam ExistingSystem The type of a system that already exists
         * @tparam T The type of the new system to replace the old one with
         * @return T& A reference to the new system
         */
        template<typename ExistingSystem, typename T>
            requires std::is_base_of_v<ExistingSystem, T> && std::is_constructible_v<T, ExistingSystem&&>
        T& replace() {
            std::unique_lock lock{mutex};

            auto nid = typeid(T).hash_code();
            auto oid = typeid(ExistingSystem).hash_code();
            auto it = systems.find(oid);
            if (it == systems.end()) {
                const auto rit = replacements.find(oid);
                if (rit == replacements.end())
                    throw std::runtime_error("Cannot replace a system that doesn't exist");
                else
                    throw std::runtime_error("In order to avoid long inheritance chains, replacing a system that already "
                                             "replaced a different one is disallowed");
            }

            auto& system = it->second;
            const auto new_system = new T{std::move(*reinterpret_cast<ExistingSystem*>(system))};
            delete system;
            system = new_system;

            T& sys = *reinterpret_cast<T*>(new_system);
            sys.should_appear_in_settings_window = std::is_same_v<decltype(&T::draw_settings_window_contents), void (T::*)()>;
            if (new_system->system_name.empty())
                new_system->system_name = typeid(T).name();

            replacements[nid] = oid;
            return sys;
        }

        ~SystemManager() {
            for (auto& [_, system] : systems) delete system;
        }
    };
} // namespace mgm
