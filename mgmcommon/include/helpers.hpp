#pragma once
#include <cmath>
#include <string>


namespace std {
    template<typename T>
    T lerp_with_delta(const T& a, const T& b, const T& speed, const T& delta_time) {
        return std::lerp(a, b, 1.0f - std::pow((T)0.5, speed * delta_time));
    }
}

template<typename, typename = void>
constexpr bool is_type_complete_v = false;

template<typename T>
constexpr inline bool is_type_complete_v <T, std::void_t<decltype(sizeof(T))>> = true;


std::string beautify_name(std::string name);


namespace mgm {
    inline bool is_whitespace(char c) {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
    }
    inline bool is_alpha(char c) {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
    }
    inline bool is_num(char c) {
        return c >= '0' && c <= '9';
    }
    inline bool is_alphanum(char c) {
        return is_num(c) || is_alpha(c);
    }
    inline bool is_sym(char c) {
        return !is_alphanum(c) && !is_whitespace(c);
    }
}
