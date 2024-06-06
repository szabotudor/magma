#pragma once
#include <cmath>


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

/// Thanks to https://stackoverflow.com/a/70701479/10978039 (by user HolyBlackCat on stack overflow) for this version of the self macro.
/// And thanks to https://github.com/MitalAshok/self_macro/ (by user MitalAshok on GitHub) for the first solution to the self macro.
/// Original implementation by MitalAshok is licenced under MIT License, as can be seen in the repository.
#if !defined(DEFINE_SELF)
namespace SelfType
    {
        void __get_self_type(...);

        template <typename T>
        struct Reader
        {
            friend auto __get_self_type(Reader<T>);
        };

        template <typename T, typename U>
        struct Writer
        {
            friend auto __get_self_type(Reader<T>){return U{};}
        };

        template <typename T>
        using Read = std::remove_pointer_t<decltype(__get_self_type(Reader<T>{}))>;
    }

#define DEFINE_SELF \
    constexpr auto __self_type_helper() -> decltype(SelfType::Writer<struct __self_type_tag, decltype(this)>{}); \
    using Self = SelfType::Read<__self_type_tag>;

#define DEFINE_SELF_WITH_NAME(name) \
    constexpr auto __##name##_type_helper() -> decltype(SelfType::Writer<struct __self_type_tag, decltype(this)>{}); \
    using name = SelfType::Read<__self_type_tag>;
#endif

