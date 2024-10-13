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



/// ==============================
/// Thanks to https://github.com/MitalAshok/self_macro/ (by user MitalAshok on GitHub) for being the first to discover the solution to the self macro.
/// Original implementation by MitalAshok is licenced under MIT License. Original license can be found on the repository linked above.
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
    constexpr auto __##name##_type_helper() -> decltype(SelfType::Writer<struct __self_type_tag, decltype(this)>{}) { return {}; } \
    using name = SelfType::Read<__self_type_tag>;
#endif
/// ==============================



std::string beautify_name(std::string name);



#define APPLY(macro, arg) macro(arg)

#define GET_ARG_COUNT(...) GET_ARG_COUNT_IMPL(__VA_ARGS__, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)
#define GET_ARG_COUNT_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, count, ...) count

#define FOR_EACH_1(macro, arg) APPLY(macro, arg)
#define FOR_EACH_2(macro, arg, ...) APPLY(macro, arg), FOR_EACH_1(macro, __VA_ARGS__)
#define FOR_EACH_3(macro, arg, ...) APPLY(macro, arg), FOR_EACH_2(macro, __VA_ARGS__)
#define FOR_EACH_4(macro, arg, ...) APPLY(macro, arg), FOR_EACH_3(macro, __VA_ARGS__)
#define FOR_EACH_5(macro, arg, ...) APPLY(macro, arg), FOR_EACH_4(macro, __VA_ARGS__)
#define FOR_EACH_6(macro, arg, ...) APPLY(macro, arg), FOR_EACH_5(macro, __VA_ARGS__)
#define FOR_EACH_7(macro, arg, ...) APPLY(macro, arg), FOR_EACH_6(macro, __VA_ARGS__)
#define FOR_EACH_8(macro, arg, ...) APPLY(macro, arg), FOR_EACH_7(macro, __VA_ARGS__)
#define FOR_EACH_9(macro, arg, ...) APPLY(macro, arg), FOR_EACH_8(macro, __VA_ARGS__)
#define FOR_EACH_10(macro, arg, ...) APPLY(macro, arg), FOR_EACH_9(macro, __VA_ARGS__)
#define FOR_EACH_11(macro, arg, ...) APPLY(macro, arg), FOR_EACH_10(macro, __VA_ARGS__)
#define FOR_EACH_12(macro, arg, ...) APPLY(macro, arg), FOR_EACH_11(macro, __VA_ARGS__)
#define FOR_EACH_13(macro, arg, ...) APPLY(macro, arg), FOR_EACH_12(macro, __VA_ARGS__)
#define FOR_EACH_14(macro, arg, ...) APPLY(macro, arg), FOR_EACH_13(macro, __VA_ARGS__)
#define FOR_EACH_15(macro, arg, ...) APPLY(macro, arg), FOR_EACH_14(macro, __VA_ARGS__)
#define FOR_EACH_16(macro, arg, ...) APPLY(macro, arg), FOR_EACH_15(macro, __VA_ARGS__)
#define FOR_EACH_17(macro, arg, ...) APPLY(macro, arg), FOR_EACH_16(macro, __VA_ARGS__)
#define FOR_EACH_18(macro, arg, ...) APPLY(macro, arg), FOR_EACH_17(macro, __VA_ARGS__)
#define FOR_EACH_19(macro, arg, ...) APPLY(macro, arg), FOR_EACH_18(macro, __VA_ARGS__)
#define FOR_EACH_20(macro, arg, ...) APPLY(macro, arg), FOR_EACH_19(macro, __VA_ARGS__)
#define FOR_EACH_21(macro, arg, ...) APPLY(macro, arg), FOR_EACH_20(macro, __VA_ARGS__)
#define FOR_EACH_22(macro, arg, ...) APPLY(macro, arg), FOR_EACH_21(macro, __VA_ARGS__)
#define FOR_EACH_23(macro, arg, ...) APPLY(macro, arg), FOR_EACH_22(macro, __VA_ARGS__)
#define FOR_EACH_24(macro, arg, ...) APPLY(macro, arg), FOR_EACH_23(macro, __VA_ARGS__)
#define FOR_EACH_25(macro, arg, ...) APPLY(macro, arg), FOR_EACH_24(macro, __VA_ARGS__)
#define FOR_EACH_26(macro, arg, ...) APPLY(macro, arg), FOR_EACH_25(macro, __VA_ARGS__)
#define FOR_EACH_27(macro, arg, ...) APPLY(macro, arg), FOR_EACH_26(macro, __VA_ARGS__)
#define FOR_EACH_28(macro, arg, ...) APPLY(macro, arg), FOR_EACH_27(macro, __VA_ARGS__)
#define FOR_EACH_29(macro, arg, ...) APPLY(macro, arg), FOR_EACH_28(macro, __VA_ARGS__)
#define FOR_EACH_30(macro, arg, ...) APPLY(macro, arg), FOR_EACH_29(macro, __VA_ARGS__)
#define FOR_EACH_31(macro, arg, ...) APPLY(macro, arg), FOR_EACH_30(macro, __VA_ARGS__)
#define FOR_EACH_32(macro, arg, ...) APPLY(macro, arg), FOR_EACH_31(macro, __VA_ARGS__)

#define FOR_EACH(macro, ...) \
    FOR_EACH_IMPL(macro, GET_ARG_COUNT(__VA_ARGS__), __VA_ARGS__)

#define FOR_EACH_IMPL(macro, count, ...) FOR_EACH_IMPL_2(macro, count, __VA_ARGS__)
#define FOR_EACH_IMPL_2(macro, count, ...) FOR_EACH_##count(macro, __VA_ARGS__)
