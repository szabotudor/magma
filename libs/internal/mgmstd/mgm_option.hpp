#pragma once
#include <variant>


namespace mgm {
    template<typename... Ts>
    using MOption = std::variant<Ts...>;


    struct MVoid {};

    template<typename T>
    using MOptional = MOption<T, MVoid>;
} // namespace mgm
