#pragma once
#include <cstdint>


#if INTPTR_MAX == INT64_MAX
using ID_t = uint64_t;
#elif INTPTR_MAX == INT32_MAX
using ID_t = uint32_t;
#endif
