#pragma once
#include "mgm_string.hpp"
#include "mgmath.hpp"


namespace mgm {
    class MgmWindow {
      public:
    };


    struct MgmWindowCreator {
        MString title{};
        vec2u32 size{};

        MgmWindow create() const;
    };
} // namespace mgm
