#pragma once
#include <deque>
#include <queue>
#include <stack>

#include "mgm_allocator.hpp"


namespace mgm {
    template<typename T, typename Allocator = MAlloc<T>>
    using MDEQueue = std::deque<T, Allocator>;

    template<typename T, typename Allocator = MAlloc<T>>
    using MQueue = std::queue<T, std::deque<T, Allocator>>;

    template<typename T, typename Allocator = MAlloc<T>>
    using MStack = std::stack<T, std::deque<T, Allocator>>;
} // namespace mgm
