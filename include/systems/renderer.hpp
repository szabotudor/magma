#pragma once
#include "systems.hpp"


namespace mgm {
    class Renderer : System {
        public:

        Renderer() {
            system_name = "Renderer";
        }

        void graphics_update() override;
    };
}
