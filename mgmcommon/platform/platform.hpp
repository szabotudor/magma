#pragma once

#if defined(__linux__) || defined(WIN32) || defined(_WIN32)
#define PLATFORM_DESKTOP
#include "platform_opengl.hpp"
#endif

#if defined(CLOSED_PLATFORM)
#include "closed_platform.hpp"
#endif
