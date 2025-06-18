#pragma once
#include <functional>
#include <unordered_map>

#include "mgm_allocator.hpp"
#include "mgm_hash.hpp"
#include "mgm_tuple.hpp"


namespace mgm {
    template<typename Key, typename T, typename Hash = MHash<Key>, typename Allocator = MAlloc<MPair<Key, T>>>
    using MMap = std::unordered_map<Key, T, Hash, std::equal_to<Key>, Allocator>;
}
