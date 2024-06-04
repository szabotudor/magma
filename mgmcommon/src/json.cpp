#include "json.hpp"
#include "logging.hpp"
#include <istream>
#include <string>
#include <unordered_map>


namespace mgm {
    bool is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
    bool is_num(char c) { return c >= '0' && c <= '9'; }
    bool is_whitespace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v'; }
    bool is_special(char c) { return !is_alpha(c) && !is_num(c) && !is_whitespace(c); }
    vec2u64 get_full_word(const std::string& str, size_t pos) {
        const auto start_pos = pos;
        const auto start = str[pos++];
        char end{};
        switch (start) {
            case '{': end = '}'; break;
            case '[': end = ']'; break;
            case '(': end = ')'; break;
            case '<': end = '>'; break;
            case '"': {
                while (pos < str.size()) {
                    if (str[pos] == '"')
                        break;
                    if (str[pos] == '\\')
                        pos++;
                    pos++;
                }
                if (pos >= str.size()) return {start_pos, start_pos};
                return {start_pos, ++pos};
            }
            default: {
                if (is_alpha(start)) {
                    size_t i = pos;
                    while (is_alpha(str[i])) i++;
                    return {start_pos, i};
                }
                if (is_num(start)) {
                    size_t i = pos;
                    while (is_num(str[i]) || str[i] == '.') i++;
                    return {start_pos, i};
                }
                if (is_whitespace(start)) {
                    size_t i = pos;
                    while (is_whitespace(str[i])) i++;
                    return {start_pos, i};
                }
            }
        }

        size_t depth = 1;
        while (depth > 0) {
            if (pos >= str.size()) return {start_pos, start_pos};
            if (str[pos] == start) depth++;
            if (str[pos] == end) depth--;
            pos++;
        }
        return {start_pos, pos};
    }



    JObject::PrivateType JObject::private_type() const {
        if (data.type() == typeid(std::string))
            return PrivateType::SINGLE;
        if (data.type() == typeid(std::vector<JObject>))
            return PrivateType::ARRAY;
        if (data.type() == typeid(std::unordered_map<std::string, JObject>))
            return PrivateType::OBJECT;
        return PrivateType::NONE;
    };

    JObject::PrivateType JObject::parsed_data_private_type() const {
        if (parsed_data.type() == typeid(std::string))
            return PrivateType::SINGLE;
        if (parsed_data.type() == typeid(std::vector<JObject>))
            return PrivateType::ARRAY;
        if (parsed_data.type() == typeid(std::unordered_map<std::string, JObject>))
            return PrivateType::OBJECT;
        return PrivateType::NONE;
    };


    std::unordered_map<std::string, JObject> &JObject::object() {
        parse();
        if (private_type() != PrivateType::OBJECT)
            data = std::unordered_map<std::string, JObject>{};
        return std::any_cast<std::unordered_map<std::string, JObject> &>(data);
    }
    const std::unordered_map<std::string, JObject> &JObject::object() const {
        if (private_type() != PrivateType::OBJECT) {
            parse();
            if (parsed_data_private_type() == PrivateType::OBJECT)
                return std::any_cast<const std::unordered_map<std::string, JObject> &>(parsed_data);
            throw std::runtime_error("Error parsing const object in JObject::object()");
        }
        return std::any_cast<const std::unordered_map<std::string, JObject> &>(data);
    }

    std::vector<JObject> &JObject::array() {
        parse();
        if (private_type() != PrivateType::ARRAY)
            data = std::vector<JObject>{};
        return std::any_cast<std::vector<JObject> &>(data);
    }
    const std::vector<JObject> &JObject::array() const {
        if (private_type() != PrivateType::ARRAY) {
            parse();
            if (parsed_data_private_type() == PrivateType::ARRAY)
                return std::any_cast<const std::vector<JObject> &>(parsed_data);
            throw std::runtime_error("Error parsing const array in JObject::array()");
        }
        return std::any_cast<const std::vector<JObject> &>(data);
    }

    std::string &JObject::single_value() {
        parse();
        if (private_type() != PrivateType::SINGLE)
            data = std::string{};
        return std::any_cast<std::string &>(data);
    }
    const std::string &JObject::single_value() const {
        if (private_type() != PrivateType::SINGLE) {
            parse();
            if (parsed_data_private_type() == PrivateType::SINGLE)
                return std::any_cast<const std::string &>(parsed_data);
            throw std::runtime_error("Error parsing const single value in JObject::single_value()");
        }
        return std::any_cast<const std::string &>(data);
    }


    std::string JObject::array_to_string() const {
        const auto& vec = array();
        std::string res = "[ ";
        for (size_t i = 0; i < vec.size(); i++) {
            res += std::string{vec[i]};
            if (i < vec.size() - 1) res += ", ";
        }
        res += " ]";
        return res;
    }
    std::string JObject::object_to_string() const {
        const auto& map = object();
        std::string res = "{ ";
        size_t i = 0;
        for (const auto& [key, value] : map) {
            if (value.type() == Type::STRING)
                res += '"' + key + "\": \"" + std::string{value} + '"';
            else
                res += '"' + key + "\": " + std::string{value};
            if (i < map.size() - 1) res += ", ";
            i++;
        }
        res += " }";
        return res;
    }

    JObject JObject::string_to_array(const std::string& str) {
        std::vector<JObject> vec;
        size_t i = 1;
        while (i < str.size() - 1) {
            size_t value_start = i;
            while (is_whitespace(str[value_start])) value_start++;
            const auto value = get_full_word(str, value_start);
            if (value.x() == value.y()) {
                Logging{"json"}.error("Broken value in JSON array, returning empty array");
                return {};
            }
            vec.emplace_back(str.substr(value.x(), value.y() - value.x()));
            i = value.y() + 1;
        }
        return vec;
    }

    JObject JObject::string_to_object(const std::string& str) {
        std::unordered_map<std::string, JObject> map;
        size_t i = 1;
        while (i < str.size() - 1) {
            if (str[i] == '}')
                break;

            const auto key_start = str.find('"', i);
            if (key_start == std::string::npos) {
                Logging{"json"}.error("No key in JSON object, returning empty object");
                return {};
            }
            const auto key = get_full_word(str, key_start);
            if (key.x() == key.y()) {
                Logging{"json"}.error("Broken key in JSON object, returning empty object");
                return {};
            }

            auto value_start = str.find(':', key.y());
            if (value_start == std::string::npos) {
                Logging{"json"}.error("No value after key in JSON object, returning empty object");
                return {};
            }
            value_start++;
            while(is_whitespace(str[value_start])) value_start++;
            auto value = get_full_word(str, value_start);
            if (value.x() == value.y()) {
                Logging{"json"}.error("Broken value in JSON object, returning empty object");
                return {};
            }

            const auto key_str = str.substr(key.x() + 1, key.y() - key.x() - 2);
            const auto value_str = str.substr(value.x(), value.y() - value.x());
            map[key_str].data = value_str;
            i = value.y() + 1;
            while (is_whitespace(str[i])) i++;
        }
        return map;
    }

    void JObject::parse() {
        if (private_type() != PrivateType::SINGLE)
            return;

        if (parsed_data_type() != Type::STRING && parsed_data_type() != Type::NULLPTR) {
            data = parsed_data;
            parsed_data.reset();
        }

        const auto str = std::any_cast<std::string>(data);
        switch (type()) {
            case Type::ARRAY: {
                data = string_to_array(str).data;
                break;
            }
            case Type::OBJECT: {
                data = string_to_object(str).data;
                break;
            }
            default: break;
        }
    }

    void JObject::parse() const {
        const auto ty = private_type();
        if (ty != PrivateType::SINGLE)
            return;
        if (type() == parsed_data_type())
            return;

        const auto str = std::any_cast<std::string>(data);
        switch (type()) {
            case Type::ARRAY: {
                parsed_data = string_to_array(str).data;
                break;
            }
            case Type::OBJECT: {
                parsed_data = string_to_object(str).data;
                break;
            }
            default: break;
        }
    }


    JObject::Type JObject::type() const {
        switch (private_type()) {
            case PrivateType::SINGLE: {
                const auto& output = single_value();
                if (output.empty()) return Type::NULLPTR;

                size_t start = 0;
                while (is_whitespace(output[start])) start++;
                size_t end = output.size() - 1;
                while (is_whitespace(output[end])) end--;
                if (start >= end) return Type::NULLPTR;

                const auto front = output[start], back = output[end];

                if (is_num(output.front())) return Type::NUMBER;
                if (front == '"' && back == '"') return Type::STRING;
                if (output == "true" || output == "false") return Type::BOOLEAN;
                if (front == '[' && back == ']') return Type::ARRAY;
                if (front == '{' && back == '}') return Type::OBJECT;
                return Type::NULLPTR;
            }
            case PrivateType::ARRAY: return Type::ARRAY;
            case PrivateType::OBJECT: return Type::OBJECT;
            default: return Type::NULLPTR;
        }
    }

    JObject::Type JObject::parsed_data_type() const {
        JObject copy{};
        copy.data = parsed_data;
        return copy.type();
    }

    JObject::JObject(const std::string& str) : data{ str } {
        switch (type()) {
            case Type::ARRAY: {
                *this = string_to_array(str);
                break;
            }
            case Type::OBJECT: {
                *this = string_to_object(str);
                break;
            }
            case Type::BOOLEAN:
            case Type::NUMBER:
            case Type::STRING: {
                break;
            }
            default: {
                data = '"' + str + '"';
                break;
            }
        }
    }

    JObject::operator std::string() const {
        switch (private_type()) {
            case PrivateType::SINGLE: {
                if (type() == Type::STRING)
                    return single_value().substr(1, single_value().size() - 2);
                return single_value();
            }
            case PrivateType::ARRAY: return array_to_string();
            case PrivateType::OBJECT: return object_to_string();
            default: return "{}";
        }
    }

    bool JObject::operator==(const JObject &other) const {
        return std::string{*this} == std::string{other};
    }
    bool JObject::operator!=(const JObject &other) const {
        return !(*this == other);
    }

    JObject& JObject::emplace_back(const JObject &value) { return array().emplace_back(value); }

    JObject& JObject::operator[](size_t index) { return array()[index]; }
    const JObject& JObject::operator[](size_t index) const { return array()[index]; }

    JObject& JObject::operator[](const std::string &key) { return object()[key]; }
    const JObject& JObject::operator[](const std::string &key) const { return object().at(key); }

    bool JObject::has(const std::string& key) const {
        if (type() != Type::OBJECT)
            return false;
        return object().find(key) != object().end();
    }

    bool JObject::has(size_t index) const {
        if (type() != Type::ARRAY)
            return false;
        return index < array().size();
    }

    void JObject::clear() {
        data.reset();
    }

    JObject::Iterator<JObject, JObject::JObjectMapIterator> JObject::begin() {
        if (type() == Type::ARRAY)
            return {this, 0};
        if (type() == Type::OBJECT) {
            const auto it = object().begin();
            if (it != object().end())
                return {this, it};
        }
        return end();
    }
    JObject::Iterator<JObject, JObject::JObjectMapIterator> JObject::end() {
        return {this, ""};
    }

    JObject::Iterator<const JObject, JObject::JObjectConstMapIterator> JObject::begin() const {
        if (type() == Type::ARRAY) {
            return {this, 0};
        }
        if (type() == Type::OBJECT) {
            const auto it = object().begin();
            if (it != object().end())
                return {this, it};
        }
        return end();
    }
    JObject::Iterator<const JObject, JObject::JObjectConstMapIterator> JObject::end() const {
        return {this, ""};
    }

    std::ostream& operator<<(std::ostream &os, const JObject &obj) {
        os << std::string{obj};
        return os;
    }
    std::istream& operator>>(std::istream &is, JObject &obj) {
        std::string str;
        is >> str;
        obj = JObject{str};
        return is;
    }
} // namespace mgm
