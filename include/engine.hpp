#pragma once
#include "logging.hpp"
#include <any>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <functional>
#include <utility>


namespace mgm {
    class MgmWindow;
    class MgmGPU;
    
    class System {
        public:
        System() = default;

        /**
         * @brief Called immediately after the system is created
         */
        virtual void on_begin_play() {}

        /**
         * @brief Called once per frame
         * 
         * @param delta Delta time for framerate (in seconds)
         */
        virtual void update(float delta) { (void)delta; /* Unused */ }

        /**
         * @brief Called once per frame, only only while the editor is open
         * 
         * @param delta Delta time for framerate (in seconds)
         */
        virtual void in_editor_update(float delta) { (void)delta; /* Unused */ }

        /**
         * @brief (Usually unused) Called once per frame, possibly on a different thread
         */
        virtual void draw() {}

        /**
         * @brief Called immediately before the system is destroyed
         */
        virtual void on_end_play() {}

        virtual ~System() = default;
    };

    class SystemManager {
        public:
        std::unordered_map<size_t, System*> systems{};

        SystemManager() = default;

        template<
            typename T,
            typename... Ts,
            std::enable_if_t<
                std::is_constructible_v<T, Ts...> && std::is_base_of_v<System, T>,
                bool
            > = true
        >
        T& create(Ts&&... args) {
            auto id = typeid(T).hash_code();
            const auto it = systems.find(id);
            if (it != systems.end()) {
                Logging{"SystemManager"}.warning("System already exists");
                return *reinterpret_cast<T*>(system);
            }
            auto& system = systems[id];
            system = reinterpret_cast<System*>(new T{std::forward<Ts>(args)...});
            T& sys = *reinterpret_cast<T*>(system);
            return sys;
        }

        template<typename T>
        T& get() {
            auto id = typeid(T).hash_code();
            const auto it = systems.find(id);
            if (it == systems.end()) {
                Logging{"SystemManager"}.error("System does not exist! Creating it now");
                return create<T>();
            }
            return *reinterpret_cast<T*>(it->second);
        }
        template<typename T>
        const T& get() const {
            auto id = typeid(T).hash_code();
            const auto it = systems.find(id);
            if (it == systems.end()) {
                Logging{"SystemManager"}.error("System does not exist!");
                throw std::runtime_error{"System does not exist"};
            }
            return *reinterpret_cast<T*>(it->second);
        }

        template<typename T>
        T* try_get() {
            auto id = typeid(T).hash_code();
            const auto it = systems.find(id);
            if (it == systems.end()) {
                return nullptr;
            }
            return reinterpret_cast<T*>(it->second);
        }
        template<typename T>
        const T* try_get() const {
            auto id = typeid(T).hash_code();
            const auto it = systems.find(id);
            if (it == systems.end()) {
                return nullptr;
            }
            return reinterpret_cast<T*>(it->second);
        }
        
        template<typename T>
        void destroy() {
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

        ~SystemManager() {
            for (auto& [_, system] : systems) {
                system->on_end_play();
                delete system;
            }
        }
    };

    class MagmaEngine {
        bool initialized = false;

        public:
        MgmWindow* window = nullptr;
        MgmGPU* graphics = nullptr;

        SystemManager systems{};

        template<typename T> T& system() { return systems.get<T>(); }
        template<typename T> const T& system() const { return systems.get<T>(); }

        MagmaEngine(const std::vector<std::string>& args = {});

        /**
         * @brief Runs the engine
         */
        void run();

        ~MagmaEngine();
    };
}
