#include "engine.hpp"
#include "file.hpp"
#include "imgui.h"
#include "imgui_impl_mgmgfx.h"
#include "mgmgfx.hpp"
#include "mgmwin.hpp"
#include <bits/chrono.h>
#include <chrono>
#include <ratio>


namespace mgm {
    MagmaEngineMainLoop::MagmaEngineMainLoop() {
        window = new MgmWindow{"Hello", {}, vec2u32{800, 600}, MgmWindow::Mode::NORMAL};
        graphics = new MgmGraphics{};
#if defined(__linux__)
        graphics->load_backend("shared/libbackend_OpenGL.so");
#elif defined(WIN32)
        graphics->load_backend("shared/backend_OpenGL.dll");
#endif
        graphics->connect_to_window(*window);
        graphics->clear_color(vec4f{0.1f, 0.2f, 0.3f, 1.0f});

        ImGui::SetCurrentContext(ImGui::CreateContext());
        ImGui_ImplMgmGFX_Init(*graphics);
    }

    void MagmaEngineMainLoop::tick(float delta) {
        window->update();

        ImGui_ImplMgmGFX_ProcessInput(*window);

        ImGui::Begin("fps", nullptr,
            ImGuiWindowFlags_NoBackground
            | ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoSavedSettings
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_AlwaysAutoResize
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoMouseInputs
        );
        ImGui::SetWindowPos({0, 0});
        ImGui::Text("FPS: %u", (uint32_t)(1.0f / delta));
        ImGui::End();

        const auto size = window->get_size();
        static vec2u32 old_size{};
        if (size.x() != old_size.x() || size.y() != old_size.y()) {
			graphics->viewport(vec2i32{0, 0}, {static_cast<int>(size.x()), static_cast<int>(size.y())});
            old_size = size;
        }
    }

    void MagmaEngineMainLoop::draw() {
        graphics->clear();
    }

    bool MagmaEngineMainLoop::running() {
        return window->is_open();
    }

    MagmaEngineMainLoop::~MagmaEngineMainLoop() {
        graphics->disconnect_from_window();
        delete graphics;
        delete window;
    }
}


int main(int argc, char** args) {
    mgm::MagmaEngineMainLoop magma{};
    
    auto start = std::chrono::high_resolution_clock::now();
    float avg_delta = 1.0f;
    constexpr auto delta_avg_calc_ratio = 0.05f;

    bool run = true;
    while (run) {
        ImGui_ImplMgmGFX_NewFrame();
        ImGui::NewFrame();

        const auto now = std::chrono::high_resolution_clock::now();
        const auto delta = std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
        start = now;

        avg_delta = avg_delta * (1.0f - delta_avg_calc_ratio) + (float)delta * 0.001f * delta_avg_calc_ratio;

        magma.tick((float)avg_delta * 0.001f);
        magma.draw();

        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplMgmGFX_RenderDrawData(ImGui::GetDrawData());
        magma.graphics->swap_buffers();

        run = magma.running();
    }

    return 0;
}
