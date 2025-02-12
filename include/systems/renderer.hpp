#pragma once
#include "built-in_components/renderable.hpp"
#include "mgmgpu.hpp"
#include "systems.hpp"
#include <mutex>


namespace mgm {
    class Renderer : public System {
      public:
        std::mutex mutex{};

        MgmGPU::Settings settings{};
        Transform camera{};
        mat4f projection{};

        Renderer();

      private:
        void gen_draw_calls(EntityComponentSystem& ecs, std::vector<MgmGPU::DrawCall>& draw_calls, MGMecs<>::Entity entity,
                            const Transform& parent_transform = {});

      public:
        void graphics_update() override;

#if defined(ENABLE_EDITOR)
        void draw_settings_window_contents() override;
#endif
    };
} // namespace mgm
