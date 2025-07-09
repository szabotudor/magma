#pragma once
#include "mgm_function.hpp"
#include "mgm_string.hpp"
#include <cstdio>


namespace mgm {
    class MAssert {
        static inline void default_on_fail(const MString& message) {
            std::fprintf(stderr, "Assertion failed: %s", message.c_str());
#if defined(_MSC_VER)
            __debugbreak();
#elif defined(__clang__) || defined(__GNUC__)
            __builtin_trap();
#else
#include <cstdlib>
            std::abort();
#endif
        }

#ifndef _NDEBUG
        MString fail_message{"ERROR"};
        MFunction<void(const MString& message)> func_on_fail = nullptr;
#endif

      public:
        MAssert() = delete;
        MAssert(MAssert&&) = delete;
        MAssert(const MAssert&) = delete;
        MAssert& operator=(MAssert&&) = delete;
        MAssert& operator=(const MAssert&) = delete;

#ifdef MAGMA_DEBUG
        inline MAssert(const MString& message = "ERROR", const MFunction<void(const MString& message)>& on_fail = nullptr)
            : fail_message(message),
              func_on_fail(on_fail) {}

        inline MAssert(const MString& message, bool) = delete;

        inline void operator()(bool cond) const {
            cond ? static_cast<void>(0) : (func_on_fail ? func_on_fail(fail_message) : default_on_fail(fail_message));
        }
#else
        inline MAssert(const MString& = "", const MFunction<void(const MString&)>& = nullptr) {}

        inline void operator()(bool) const {}
#endif
    };
} // namespace mgm
