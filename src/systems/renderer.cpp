#include "systems/renderer.hpp"
#include "backend_settings.hpp"
#include "built-in_components/renderable.hpp"
#include "ecs.hpp"
#include "engine.hpp"
#include "mgmgpu.hpp"
#include "systems/resources.hpp"
#include "tools/mgmecs.hpp"


namespace mgm {
    thread_local mat4f current_cam_transform{};

    void Renderer::gen_draw_calls(EntityComponentSystem& ecs, std::vector<MgmGPU::DrawCall>& draw_calls, MGMecs<>::Entity entity, const Transform& parent_transform) {
        for (const auto& e : ecs.ecs.get<HierarchyNode>(entity)) {
            ecs.ecs.wait_and_lock(e);

            const auto transform = ecs.ecs.try_get<Transform>(e);
            if (transform == nullptr) {
                ecs.ecs.unlock(e);
                continue;
            }

            const auto mesh = ecs.ecs.try_get<ResourceReference<Mesh>>(e);
            if (mesh == nullptr || !mesh->valid() || !mesh->get().shader.valid()) {
                ecs.ecs.unlock(e);
                continue;
            }

            const auto local_transform = parent_transform * *transform;

            draw_calls.emplace_back(MgmGPU::DrawCall{
                .type = MgmGPU::DrawCall::Type::DRAW,
                .shader = mesh->get().shader.get().created_shader,
                .buffers_object = mesh->get().buffers_object,
                .textures = {}, // TODO: Add textures in meshes
                .parameters = {
                    {"transform", local_transform.as_matrix()},
                    {"camera", current_cam_transform},
                    {"proj", projection}
                }
            });

            gen_draw_calls(ecs, draw_calls, e, local_transform);

            ecs.ecs.unlock(e);
        }
    }

    Renderer::Renderer() {
        system_name = "Renderer";

        settings.backend.clear.color = {0.0f, 1.0f, 0.0f, 1.0f};
        settings.backend.depth_testing.enabled = true;
        settings.backend.culling.type = GPUSettings::Culling::Type::CLOCKWISE;

        projection = mat4f::gen_perspective_projection(90.0f, 9.0f/16.0f, 0.1f, 1000.0f);
    }

    void Renderer::graphics_update() {
        auto& ecs = MagmaEngine{}.ecs();

        std::vector<MgmGPU::DrawCall> draw_calls{};

        mutex.lock();
        const auto use_settings = settings;
        mutex.unlock();

        if (use_settings.canvas != MgmGPU::INVALID_TEXTURE) {
            draw_calls.emplace_back(MgmGPU::DrawCall{
                .type = MgmGPU::DrawCall::Type::CLEAR
            });
        }

        #if defined(ENABLE_EDITOR)
        MGMecs<>::Entity scene{};
        if (MagmaEngine{}.editor().is_running())
            scene = ecs.root;
        else
            scene = ecs.current_editing_scene;
        #else
        const auto scene = ecs.root;
        #endif

        if (scene != MGMecs<>::null) {
            ecs.ecs.wait_and_lock(scene);
            current_cam_transform = camera.as_matrix();
            gen_draw_calls(ecs, draw_calls, scene);
            ecs.ecs.unlock(scene);
        }

        MagmaEngine{}.graphics().draw(draw_calls, use_settings);
    }

    #if defined(ENABLE_EDITOR)
    void Renderer::draw_settings_window_contents() {
        ImGui::Text("Renderer Settings"); // TODO: Implement settings
    }
    #endif
}
