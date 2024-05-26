#pragma once
#include "logging.hpp"
#include <any>
#include <string>
#include <cstdint>
#include <unordered_map>


namespace mgm {
    class JObject {
        enum class PrivateType {
            NONE, SINGLE, ARRAY, OBJECT
        };
        PrivateType private_type() const;

        std::any data{};
        std::unordered_map<std::string, JObject> &object();
        const std::unordered_map<std::string, JObject> &object() const;
        std::vector<JObject> &array();
        const std::vector<JObject> &array() const;
        std::string &single_value();
        const std::string &single_value() const;

        std::string array_to_string() const;
        std::string object_to_string() const;
        static JObject string_to_array(const std::string& str);
        static JObject string_to_object(const std::string& str);

        void parse();

        public:
        enum class Type {
            NUMBER,
            STRING,
            BOOLEAN,
            ARRAY,
            OBJECT,
            NULLPTR
        };
        Type type() const;

        JObject() = default;
        JObject(const JObject& other) : data{other.data} {}
        JObject(JObject&& other) : data{std::move(other.data)} {}
        JObject& operator=(const JObject& other) {
            if (this == &other) 
                return *this;
            data = other.data;
            return *this;
        }
        JObject& operator=(JObject&& other) {
            if (this == &other) 
                return *this;
            data = std::move(other.data);
            return *this;
        }

        JObject(const std::string& str);
        JObject(const char* str) : JObject{ std::string{str} } {}
        JObject(const char* begin, const char* end) : JObject{ std::string{begin, end} } {}
        JObject(const int32_t i) : data{ std::to_string(i) } {}
        JObject(const uint32_t i) : data{ std::to_string(i) } {}
        JObject(const int64_t i) : data{ std::to_string(i) } {}
        JObject(const uint64_t i) : data{ std::to_string(i) } {}
        JObject(const float f) : data{ std::to_string(f) } {}
        JObject(const double d) : data{ std::to_string(d) } {}
        JObject(const bool b) : data{ b ? std::string{"true"} : std::string{"false"} } {}
        JObject(const std::vector<JObject>& vec) : data{ vec } {}
        JObject(const std::unordered_map<std::string, JObject>& map) : data{ map } {}

        operator std::string() const;
        explicit operator int32_t() const { return std::stoi(single_value()); }
        explicit operator uint32_t() const { return std::stoul(single_value()); }
        explicit operator int64_t() const { return std::stoll(single_value()); }
        explicit operator uint64_t() const { return std::stoull(single_value()); }
        explicit operator float() const { return std::stof(single_value()); }
        explicit operator double() const { return std::stod(single_value()); }
        explicit operator bool() const { return single_value() == "true"; }
        operator std::vector<JObject>() const { return array(); }
        operator std::unordered_map<std::string, JObject>() const { return object(); }

        bool operator==(const JObject& other) const;
        bool operator!=(const JObject& other) const;


        /**
         * @brief Interpret the JObject as an array, and add a new element to the end
         * 
         * @param value The value to add to the array
         * @return JObject& A reference to the new element
         */
        JObject &emplace_back(const JObject &value);

        /**
         * @brief Index into the JObject as an array
         * 
         * @param index The index to access
         * @return JObject& A reference to the element at the given index
         */
        JObject& operator[](size_t index);

        /**
         * @brief Index into the JObject as an array
         * 
         * @param index The index to access
         * @return const JObject& A const reference to the element at the given index
         */
        const JObject& operator[](size_t index) const;

        /**
         * @brief Index into the JObject as an object
         * 
         * @param key The key to access
         * @return JObject& A reference to the element with the given key
         */
        JObject& operator[](const std::string& key);

        /**
         * @brief Index into the JObject as an object
         * 
         * @param key The key to access
         * @return const JObject& A const reference to the element with the given key
         */
        const JObject& operator[](const std::string& key) const;

        /**
         * @brief If the JObject is an object, check if it has a key
         * 
         * @param key The key to check for
         * @return true If the key exists
         */
        bool has(const std::string& key) const;

        /**
         * @brief If the JObject is an array, check if it has an index
         * 
         * @param index The index to check for
         * @return true If the index exists
         */
        bool has(size_t index) const;

        /**
         * @brief Clear the JObject, setting it to an empty state
         */
        void clear();

        struct Iterator;
        struct ConstIterator;

        Iterator begin();
        Iterator end();

        ConstIterator begin() const;
        ConstIterator end() const;

        friend std::ostream& operator<<(std::ostream& os, const JObject& obj);
        friend std::istream& operator>>(std::istream& is, JObject& obj);
    };

    struct JObject::Iterator {
        friend class JObject;

        using iterator_category = std::forward_iterator_tag;
        using value_type = JObject;
        using difference_type = std::ptrdiff_t;
        using pointer = JObject*;
        using reference = JObject&;
        
        JObject* obj = nullptr;
        JObject key{};

        Iterator() = default;
        Iterator(JObject* obj, JObject key) : obj{obj}, key{key} {}

        struct Deref {
            JObject key;
            JObject& val;
        };

        Deref operator*() {
            switch (key.type()) {
                case JObject::Type::NUMBER: return {.key = key, .val = (*obj)[static_cast<size_t>(key)]};
                case JObject::Type::STRING: return {.key = key, .val = (*obj)[std::string{key}]};
                default: return {.key = key, .val = *obj};
            }
        }
        Deref operator->() { return operator*(); }

        Iterator& operator++() {
            if (key.type() == JObject::Type::NUMBER) {
                const auto new_key = static_cast<size_t>(key) + 1;
                if (new_key >= obj->array().size())
                    key = "";
                else
                    key = JObject{new_key};
            } else {
                auto& obj_map = obj->object();
                auto it = obj_map.find(std::string{key});
                if (it == obj_map.end()) {
                    key = "";
                } else {
                    auto next = std::next(it);
                    if (next == obj_map.end()) {
                        key = "";
                    } else {
                        key = next->first;
                    }
                }
            }
            return *this;
        }
        Iterator operator++(int) {
            auto copy = *this;
            operator++();
            return copy;
        }

        bool operator==(const Iterator& other) const {
            return obj == other.obj && key == other.key;
        }
        bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }
    };

    struct JObject::ConstIterator {
        friend class JObject;

        using iterator_category = std::forward_iterator_tag;
        using value_type = const JObject;
        using difference_type = std::ptrdiff_t;
        using pointer = const JObject*;
        using reference = const JObject&;
        
        const JObject* obj = nullptr;
        JObject key{};

        ConstIterator() = default;
        ConstIterator(const JObject* obj, JObject key) : obj{obj}, key{key} {}

        struct Deref {
            JObject key;
            const JObject& val;
        };

        Deref operator*() {
            switch (key.type()) {
                case JObject::Type::NUMBER: return {.key = key, .val = (*obj)[static_cast<size_t>(key)]};
                case JObject::Type::STRING: return {.key = key, .val = (*obj)[std::string{key}]};
                default: return {.key = key, .val = *obj};
            }
        }
        Deref operator->() { return operator*(); }

        ConstIterator& operator++() {
            if (key.type() == JObject::Type::NUMBER) {
                const auto new_key = static_cast<size_t>(key) + 1;
                if (new_key >= obj->array().size())
                    key = "";
                else
                    key = JObject{new_key};
            } else {
                auto& obj_map = obj->object();
                auto it = obj_map.find(std::string{key});
                if (it == obj_map.end()) {
                    key = "";
                } else {
                    auto next = std::next(it);
                    if (next == obj_map.end()) {
                        key = "";
                    } else {
                        key = next->first;
                    }
                }
            }
            return *this;
        }
        ConstIterator operator++(int) {
            auto copy = *this;
            operator++();
            return copy;
        }

        bool operator==(const ConstIterator& other) const {
            return obj == other.obj && key == other.key;
        }
        bool operator!=(const ConstIterator& other) const {
            return !(*this == other);
        }
    };
}
