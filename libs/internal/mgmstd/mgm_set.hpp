#pragma once
#include <functional>
#include <unordered_set>

#include "mgm_allocator.hpp"
#include "mgm_hash.hpp"


namespace mgm {
    template<typename T, typename Hash = MHash<T>, typename Allocator = MAlloc<T>>
    using MSet = std::unordered_set<T, Hash, std::equal_to<T>, Allocator>;
}
