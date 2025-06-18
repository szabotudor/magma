#pragma once
#include <memory>


namespace mgm {
    template<typename T>
    using MAlloc = std::allocator<T>;
}
