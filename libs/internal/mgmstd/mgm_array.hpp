#pragma once
#include <vector>

#include "mgm_allocator.hpp"


namespace mgm {
    template<typename T, typename Allocator = MAlloc<T>>
    using MArray = std::vector<T, Allocator>;
}
