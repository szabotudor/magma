#pragma once
#include <cmath>


namespace std {
    template<typename T>
    T lerp_with_delta(const T& a, const T& b, const T& speed, const T& delta_time) {
        return std::lerp(a, b, 1.0f - std::pow((T)0.5, speed * delta_time));
    }
}
