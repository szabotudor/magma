#include "engine.hpp"
#include "backend_settings.hpp"
#include "imgui.h"
#include "imgui_impl_mgmgpu.h"
#include "logging.hpp"
#include "mgmgpu.hpp"
#include "mgmwin.hpp"

#include <chrono>
#include <iostream>


namespace mgm {
    MagmaEngine::MagmaEngine() {
        window = new MgmWindow{"Hello", vec2u32{800, 600}, MgmWindow::Mode::NORMAL};
        graphics = new MgmGPU{};
        graphics->connect_to_window(window);
#if defined(__linux__)
        graphics->load_backend("shared/libbackend_OpenGL.so");
#elif defined (WIN32) || defined(_WIN32)
        graphics->load_backend("shared/backend_OpenGL.dll");
#endif

        auto& settings = graphics->settings();
        settings.clear.enabled = true;
        settings.clear.color = {0.1f, 0.2f, 0.3f, 1.0f};
        settings.viewport.top_left = {0, 0};
        settings.viewport.bottom_right = vec2i32{static_cast<int>(window->get_size().x()), static_cast<int>(window->get_size().y())};

        graphics->apply_settings();
    }

    void MagmaEngine::init() {
        ShaderCreateInfo shader_info{};
        shader_info.shader_sources.emplace_back(ShaderCreateInfo::SingleShaderInfo{
            ShaderCreateInfo::SingleShaderInfo::Type::VERTEX,
            "#version 460 core\n"
            "layout(location = 0) in vec3 aPos;\n"
            "void main() {\n"
            "    gl_Position = vec4(aPos, 1.0f);\n"
            "}\n"
        });
        shader_info.shader_sources.emplace_back(ShaderCreateInfo::SingleShaderInfo{
            ShaderCreateInfo::SingleShaderInfo::Type::FRAGMENT,
            "#version 460 core\n"
            "out vec4 FragColor;\n"
            "void main() {\n"
            "    FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);\n"
            "}\n"
        });

        const auto shader = graphics->create_shader(shader_info);
        if (shader == MgmGPU::INVALID_SHADER) {
            Logging{"main"}.log("Failed to create shader");
            return;
        }
        
        static const vec3f vertices[] = {
            {-0.5f, -0.5f, 0.0f},
            { 0.5f, -0.5f, 0.0f},
            { 0.0f,  0.5f, 0.0f}
        };
        const BufferCreateInfo buffer_info{BufferCreateInfo::Type::RAW, vertices, sizeof(vertices) / sizeof(vec3f)};
        const auto buffer = graphics->create_buffer(buffer_info);
        if (buffer == MgmGPU::INVALID_BUFFER) {
            Logging{"main"}.log("Failed to create buffer");
            return;
        }
        
        const auto buffers_object = graphics->create_buffers_object({buffer});

        MgmGPU::DrawCall draw_call{
            MgmGPU::DrawCall::Type::DRAW,
            shader,
            buffers_object,
            {},
            {}
        };

        graphics->draw_list.emplace_back(MgmGPU::DrawCall{
            .type = MgmGPU::DrawCall::Type::CLEAR
        });

        graphics->draw_list.emplace_back(draw_call);
    }

    void MagmaEngine::tick(float delta) {
        constexpr auto window_falgs = ImGuiWindowFlags_AlwaysAutoResize
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoBackground
            | ImGuiWindowFlags_NoSavedSettings;

        ImGui::SetNextWindowPos({0, 0});
        ImGui::Begin("Debug", nullptr, window_falgs);
        ImGui::Text("%.2f", 1.0f / delta);
        ImGui::End();
    }

    void MagmaEngine::draw() {
        graphics->draw();
    }

    void MagmaEngine::close() {
        delete graphics;
        delete window;
    }
}

int main() {
    mgm::MagmaEngine magma{};

    ImGui_ImplMgmGFX_Init(*magma.graphics);
    magma.init();

    auto start = std::chrono::high_resolution_clock::now();
    float avg_delta = 1.0f;

    bool run = true;
    while (run) {
        constexpr auto delta_avg_calc_ratio = 0.05f;
        const auto now = std::chrono::high_resolution_clock::now();
        const auto delta = std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
        start = now;

        avg_delta = avg_delta * (1.0f - delta_avg_calc_ratio) + (float)delta * 0.001f * delta_avg_calc_ratio;

        magma.window->update();
        ImGui_ImplMgmGFX_ProcessInput(*magma.window);
        ImGui_ImplMgmGFX_NewFrame();
        ImGui::NewFrame();

        magma.tick(avg_delta * 0.001f);

        ImGui::EndFrame();
        ImGui::Render();

        magma.draw();
        ImGui_ImplMgmGFX_RenderDrawData(ImGui::GetDrawData());
        magma.graphics->present();

        run = !magma.window->should_close();
    }

    ImGui_ImplMgmGFX_Shutdown();

    magma.close();
    mgm::Logging{"main"}.log("Closed engine");
    return 0;
}
