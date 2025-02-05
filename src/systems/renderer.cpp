#include "systems/renderer.hpp"
#include "built-in_components/renderable.hpp"
#include "ecs.hpp"
#include "engine.hpp"
#include "mgmgpu.hpp"
#include "tools/mgmecs.hpp"


namespace mgm {
    void Renderer::graphics_update() {
        auto& ecs = MagmaEngine{}.ecs();
        
        const auto group = ecs.ecs.group<Mesh, MGMecs<>::Include<Transform>>();

        std::vector<MgmGPU::DrawCall> draw_calls{};

        for (const auto& e : group) {
            auto& mesh = e.second;
            auto& transform = ecs.ecs.get<Transform>(e.first);

            draw_calls.emplace_back(MgmGPU::DrawCall{
                .type = MgmGPU::DrawCall::Type::DRAW,
                .shader = mesh.shader.get().created_shader,
                .buffers_object = mesh.buffers_object,
                .parameters = {
                    {"Transform", transform},
                    {"CamTransform", 0}
                }
            });
        }
    }
}
