#pragma once
#include <cassert>
#include <functional>


namespace mgm {
    struct Any {
        void* value = nullptr;
        size_t type_id = 0;
        std::function<void(void*)> destructor = nullptr;
        std::function<void(void*&, void*)> copy_constructor = nullptr;

        bool empty() const {
            return value == nullptr;
        }

        Any() = default;
        Any(const Any& other) {
            if (other.empty())
                return;
            type_id = other.type_id;
            destructor = other.destructor;
            copy_constructor = other.copy_constructor;
            copy_constructor(value, other.value);
        }
        Any(Any&& other) {
            if (other.empty())
                return;
            type_id = other.type_id;
            destructor = other.destructor;
            copy_constructor = other.copy_constructor;
            value = other.value;
            other.value = nullptr;
            other.type_id = 0;
            other.destructor = nullptr;
            other.copy_constructor = nullptr;
        }
        Any& operator=(const Any& other) {
            if (this == &other)
                return *this;
            destroy();
            if (other.empty())
                return *this;
            type_id = other.type_id;
            destructor = other.destructor;
            copy_constructor = other.copy_constructor;
            copy_constructor(value, other.value);
            return *this;
        }
        Any& operator=(Any&& other) {
            if (this == &other)
                return *this;
            destroy();
            if (other.empty())
                return *this;
            type_id = other.type_id;
            destructor = other.destructor;
            copy_constructor = other.copy_constructor;
            value = other.value;
            other.value = nullptr;
            other.type_id = 0;
            other.destructor = nullptr;
            other.copy_constructor = nullptr;
            return *this;
        }

        template<typename T>
        Any(T&& value_to_copy) {
            emplace<T>(std::forward<T>(value_to_copy));
        }
        
        template<typename T, typename... Ts, std::enable_if_t<std::is_constructible_v<T, Ts...>, bool> = true>
        void emplace(Ts&&... args) {
            if constexpr(std::is_same_v<T, void>)
                return;
            value = new T(std::forward<Ts>(args)...);
            type_id = typeid(T).hash_code();
            destructor = [](void* ptr) { delete static_cast<T*>(ptr); };
            copy_constructor = [](void*& dest, void* src) { dest = new T(*static_cast<T*>(src)); };
        }

        template<typename T>
        T& get() {
            assert(type_id == typeid(T).hash_code() && "Any: type mismatch");
            return *static_cast<T*>(value);
        }
        template<typename T>
        const T& get() const {
            assert(type_id == typeid(T).hash_code() && "Any: type mismatch");
            return *static_cast<T*>(value);
        }

        void destroy() {
            if (destructor != nullptr)
                destructor(value);
            value = nullptr;
            type_id = 0;
            destructor = nullptr;
        }

        ~Any() {
            destroy();
        }
    };
}
