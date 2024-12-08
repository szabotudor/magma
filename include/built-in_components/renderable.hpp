#pragma once
#include "json.hpp"
#include "mgmath.hpp"


namespace mgm {
    template<typename T> vec<1, T> to_vec1(const JObject& json) { return vec<1, float>{T(json["x"])}; }
    template<typename T> vec<2, T> to_vec2(const JObject& json) { return vec<2, float>{T(json["x"]), T(json["y"])}; }
    template<typename T> vec<3, T> to_vec3(const JObject& json) { return vec<3, float>{T(json["x"]), T(json["y"]), T(json["z"])}; }
    template<typename T> vec<4, T> to_vec4(const JObject& json) { return vec<4, float>{T(json["x"]), T(json["y"]), T(json["z"]), T(json["w"])}; }

    template<size_t S, typename T> vec<S, T> to_vec(const JObject& json) {
        if constexpr (S == 1)
            return to_vec1<T>(json);
        if constexpr (S == 2)
            return to_vec2<T>(json);
        if constexpr (S == 3)
            return to_vec3<T>(json);
        if constexpr (S == 4)
            return to_vec4<T>(json);
    }

    template<typename T> JObject from_vec1(const vec<1, T>& vec) { 
        JObject json; 
        json["x"] = vec.x(); 
        return json; 
    }

    template<typename T> JObject from_vec2(const vec<2, T>& vec) { 
        JObject json; 
        json["x"] = vec.x(); 
        json["y"] = vec.y(); 
        return json; 
    }

    template<typename T> JObject from_vec3(const vec<3, T>& vec) { 
        JObject json; 
        json["x"] = vec.x(); 
        json["y"] = vec.y(); 
        json["z"] = vec.z(); 
        return json; 
    }

    template<typename T> JObject from_vec4(const vec<4, T>& vec) { 
        JObject json; 
        json["x"] = vec.x(); 
        json["y"] = vec.y(); 
        json["z"] = vec.z(); 
        json["w"] = vec.w(); 
        return json; 
    }

    template<size_t S, typename T> JObject from_vec(const vec<S, T>& vec) {
        if constexpr (S == 1)
            return from_vec1<T>(vec);
        if constexpr (S == 2)
            return from_vec2<T>(vec);
        if constexpr (S == 3)
            return from_vec3<T>(vec);
        if constexpr (S == 4)
            return from_vec4<T>(vec);
    }


    struct Transform {
        vec3f pos{};
        vec3f scale{};
        vec4f rot{};

        Transform() = default;

        Transform(const JObject& json);
        operator JObject();
    };
}
