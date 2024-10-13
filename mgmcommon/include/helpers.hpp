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
/// Original implementation by MitalAshok is licensed under MIT License. Original license can be found on the repository linked above.
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


//==================================================================================================
// Thanks to Luiz Martins for the VAR_COUNT macro, in this answer on stackoverflow: https://stackoverflow.com/a/66556553
/* NOTE: In these macros, "1" means true, and "0" means false. */

#define EXPAND(x) x

#define _GLUE(X,Y) X##Y
#define GLUE(X,Y) _GLUE(X,Y)

/* Returns the 100th argument. */
#define _ARG_100(_,\
   _100,_99,_98,_97,_96,_95,_94,_93,_92,_91,_90,_89,_88,_87,_86,_85,_84,_83,_82,_81, \
   _80,_79,_78,_77,_76,_75,_74,_73,_72,_71,_70,_69,_68,_67,_66,_65,_64,_63,_62,_61, \
   _60,_59,_58,_57,_56,_55,_54,_53,_52,_51,_50,_49,_48,_47,_46,_45,_44,_43,_42,_41, \
   _40,_39,_38,_37,_36,_35,_34,_33,_32,_31,_30,_29,_28,_27,_26,_25,_24,_23,_22,_21, \
   _20,_19,_18,_17,_16,_15,_14,_13,_12,_11,_10,_9,_8,_7,_6,_5,_4,_3,_2,X_,...) X_

/* Returns whether __VA_ARGS__ has a comma (up to 100 arguments). */
#define HAS_COMMA(...) EXPAND(_ARG_100(__VA_ARGS__, \
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, \
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 ,1, \
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0))

/* Produces a comma if followed by a parenthesis. */
#define _TRIGGER_PARENTHESIS_(...) ,
#define _PASTE5(_0, _1, _2, _3, _4) _0 ## _1 ## _2 ## _3 ## _4
#define _IS_EMPTY_CASE_0001 ,
/* Returns true if inputs expand to (false, false, false, true) */
#define _IS_EMPTY(_0, _1, _2, _3) HAS_COMMA(_PASTE5(_IS_EMPTY_CASE_, _0, _1, _2, _3))
/* Returns whether __VA_ARGS__ is empty. */
#define IS_EMPTY(...)                                               \
   _IS_EMPTY(                                                       \
      /* Testing for an argument with a comma                       \
         e.g. "ARG1, ARG2", "ARG1, ...", or "," */                  \
      HAS_COMMA(__VA_ARGS__),                                       \
      /* Testing for an argument around parenthesis                 \
         e.g. "(ARG1)", "(...)", or "()" */                         \
      HAS_COMMA(_TRIGGER_PARENTHESIS_ __VA_ARGS__),                 \
      /* Testing for a macro as an argument, which will             \
         expand the parenthesis, possibly generating a comma. */    \
      HAS_COMMA(__VA_ARGS__ (/*empty*/)),                           \
      /* If all previous checks are false, __VA_ARGS__ does not     \
         generate a comma by itself, nor with _TRIGGER_PARENTHESIS_ \
         behind it, nor with () after it.                           \
         Therefore, "_TRIGGER_PARENTHESIS_ __VA_ARGS__ ()"          \
         only generates a comma if __VA_ARGS__ is empty.            \
         So, this tests for an empty __VA_ARGS__ (given the         \
         previous conditionals are false). */                       \
      HAS_COMMA(_TRIGGER_PARENTHESIS_ __VA_ARGS__ (/*empty*/))      \
   )

#define _VAR_COUNT_EMPTY_1(...) 0
#define _VAR_COUNT_EMPTY_0(...) EXPAND(_ARG_100(__VA_ARGS__, \
   100,99,98,97,96,95,94,93,92,91,90,89,88,87,86,85,84,83,82,81, \
   80,79,78,77,76,75,74,73,72,71,70,69,68,67,66,65,64,63,62,61, \
   60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41, \
   40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21, \
   20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1))
#define VAR_COUNT(...) GLUE(_VAR_COUNT_EMPTY_, IS_EMPTY(__VA_ARGS__))(__VA_ARGS__)
//==================================================================================================


#define FOR_EACH_0(macro, arg)
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
    FOR_EACH_IMPL(macro, VAR_COUNT(__VA_ARGS__), __VA_ARGS__)

#define FOR_EACH_IMPL(macro, count, ...) FOR_EACH_IMPL_1(macro, count, __VA_ARGS__)
#define FOR_EACH_IMPL_1(macro, count, ...) FOR_EACH_##count(macro, __VA_ARGS__)
