#pragma once
#include <tuple>
#include <utility>


namespace mgm {
    template<typename... Ts>
    using MTuple = std::tuple<Ts...>;

    template<typename T, typename U>
    using MPair = std::pair<T, U>;
} // namespace mgm
