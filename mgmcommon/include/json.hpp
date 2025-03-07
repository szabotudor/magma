#pragma once
#include <any>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>


#define JSON_SINGLE_INDENT "    "


namespace mgm {
    class JObject {
        enum class PrivateType {
            NONE,
            SINGLE,
            ARRAY,
            OBJECT
        };
        PrivateType private_type() const;
        PrivateType parsed_data_private_type() const;

        std::any data{};
        mutable std::any parsed_data{};

      public:
        /**
         * @brief Interpret the JObject as a json object and return it as a map, clearning the original value if not already an object type
         */
        std::unordered_map<std::string, JObject>& object();

        /**
         * @brief Try to interpret the JObject as a json object and return it as a map, and throw an error if not already an object type
         */
        const std::unordered_map<std::string, JObject>& object() const;

        /**
         * @brief Interpret the JObject as a json object and return it as a vector, clearning the original value if not already an array type
         */
        std::vector<JObject>& array();

        /**
         * @brief Try to interpret the JObject as a json object and return it as a vector, and throw an error if not already an array type
         */
        const std::vector<JObject>& array() const;

      private:
        std::string& single_value();
        const std::string& single_value() const;

        static inline thread_local size_t indent = 1;

        std::string array_to_string() const;
        std::string object_to_string() const;
        static JObject string_to_array(const std::string& str);
        static JObject string_to_object(const std::string& str);

        void parse();
        void parse() const;

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

      private:
        Type parsed_data_type() const;

      public:
        JObject() = default;
        JObject(const JObject& other)
            : data{other.data} {}
        JObject(JObject&& other)
            : data{std::move(other.data)} {}
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
        JObject(const char* str)
            : JObject{std::string{str}} {}
        JObject(const char* begin, const char* end)
            : JObject{
                  std::string{begin, end}
        } {}
        JObject(const int32_t i)
            : data{std::to_string(i)} {}
        JObject(const uint32_t i)
            : data{std::to_string(i)} {}
        JObject(const int64_t i)
            : data{std::to_string(i)} {}
        JObject(const uint64_t i)
            : data{std::to_string(i)} {}
        JObject(const float f)
            : data{std::to_string(f)} {}
        JObject(const double d)
            : data{std::to_string(d)} {}
        JObject(const bool b)
            : data{b ? std::string{"true"} : std::string{"false"}} {}
        JObject(const std::vector<JObject>& vec)
            : data{vec} {}
        JObject(const std::unordered_map<std::string, JObject>& map)
            : data{map} {}

        operator std::string() const;
        explicit operator int32_t() const { return std::stoi(single_value()); }
        explicit operator uint32_t() const { return (uint32_t)std::stoul(single_value()); }
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
         * @brief Check if this JObject is empty
         */
        bool empty() const;

        /**
         * @brief Check if this JObject is a decimal number
         *
         * @return true If the JObject is a number, and is decimal
         */
        bool is_number_decimal();

        /**
         * @brief Interpret the JObject as an array, and add a new element to the end
         *
         * @param value The value to add to the array
         * @return JObject& A reference to the new element
         */
        JObject& emplace_back(const JObject& value);

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

        template<typename T, typename MapIterator>
        struct Iterator;

        using JObjectMapIterator = std::unordered_map<std::string, JObject>::iterator;
        using JObjectConstMapIterator = std::unordered_map<std::string, JObject>::const_iterator;

        Iterator<JObject, JObjectMapIterator> begin();
        Iterator<JObject, JObjectMapIterator> end();

        Iterator<const JObject, JObjectConstMapIterator> begin() const;
        Iterator<const JObject, JObjectConstMapIterator> end() const;

        friend std::ostream& operator<<(std::ostream& os, const JObject& obj);
        friend std::istream& operator>>(std::istream& is, JObject& obj);
    };

    template<typename T, typename MapIterator>
    struct JObject::Iterator {
        friend class JObject;

        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        pointer obj = nullptr;
        std::variant<MapIterator, size_t> key{};

        Iterator() = default;
        Iterator(pointer object, size_t member_key)
            : obj{object},
              key{member_key} {}
        Iterator(pointer object, MapIterator member_key)
            : obj{object},
              key{member_key} {}

        struct Deref {
            JObject key;
            reference val;
        };

        Deref operator*() {
            if (std::holds_alternative<MapIterator>(key))
                return {.key = std::get<MapIterator>(key)->first, .val = std::get<MapIterator>(key)->second};
            if (std::holds_alternative<size_t>(key))
                return {.key = static_cast<size_t>(std::get<size_t>(key)), .val = (*obj)[static_cast<size_t>(std::get<size_t>(key))]};
            throw std::runtime_error{"Invalid iterator"};
        }
        Deref operator->() { return operator*(); }

        Iterator& operator++() {
            if (std::holds_alternative<MapIterator>(key)) {
                const auto& obj_map = obj->object();
                auto it = std::get<MapIterator>(key);
                if (it == obj_map.end())
                    key = static_cast<size_t>(-1);
                else {
                    auto next = std::next(it);
                    if (next == obj_map.end())
                        key = static_cast<size_t>(-1);
                    else
                        key = next;
                }
            }
            else if (std::holds_alternative<size_t>(key)) {
                const auto new_key = std::get<size_t>(key) + 1;
                if (new_key >= obj->array().size())
                    key = static_cast<size_t>(-1);
                else
                    key = new_key;
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
} // namespace mgm
