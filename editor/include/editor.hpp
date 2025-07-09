#pragma once
#include "mgm_backend.hpp"
#include "mgm_window.hpp"


namespace mgm {
    class MagmaEditor {
        MgmWindow window{
            {800, 600},
            "Magma Editor"
        };
        MgmGraphics backend{};

      public:
        MagmaEditor();

        void run();
    };
} // namespace mgm
