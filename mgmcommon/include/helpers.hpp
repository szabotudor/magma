#pragma once
#include <cmath>
#include <string>


namespace std {
    template<typename T>
    T lerp_with_delta(const T& a, const T& b, const T& speed, const T& delta_time) {
        return std::lerp(a, b, 1.0f - std::pow((T)0.5, speed * delta_time));
    }
} // namespace std

template<typename, typename = void>
constexpr bool is_type_complete_v = false;

template<typename T>
constexpr inline bool is_type_complete_v<T, std::void_t<decltype(sizeof(T))>> = true;


std::string beautify_name(std::string name);
