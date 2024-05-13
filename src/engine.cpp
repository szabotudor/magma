#include "engine.hpp"
#include "backend_settings.hpp"
#include "editor.hpp"
#include "imgui.h"
#include "imgui_impl_mgmgpu.h"
#include "logging.hpp"
#include "mgmgpu.hpp"
#include "mgmwin.hpp"

#include <chrono>
#include <iostream>
#include <unordered_map>


namespace mgm {
    MagmaEngine* MagmaEngine::instance = nullptr;

    MagmaEngine::MagmaEngine(const std::vector<std::string>& args) {
        if (!instance) {
            instance = this;
        } else {
            window = instance->window;
            graphics = instance->graphics;
            system_manager = instance->system_manager;
            initialized = true;
            return;
        }

        system_manager = new SystemManager{};

        bool help_called = false;
        std::unordered_map<std::string, std::function<void(MagmaEngine*)>> args_map{
            {"--help", [&](MagmaEngine* engine) {
                (void)engine;
                std::cout << "Usage: " << "magma [options]\n"
                    << "Options:\n"
                    << "\t--help\t\tShow this help message\n"
                    << "\t--editor\tStart the editor\n";
                help_called = true;
            }},
#if defined(ENABLE_EDITOR)
            {"--editor", [](MagmaEngine* engine) {
                engine->systems().create<Editor>();
            }}
#endif
        };

        for (const auto& arg : args) {
            const auto it = args_map.find(arg);
            if (it == args_map.end()) {
                Logging{"main"}.warning("Unknown argument: ", arg);
                continue;
            }
            it->second(this);
        }

        if (help_called) return;


        window = new MgmWindow{"Magma", vec2u32{800, 600}, MgmWindow::Mode::NORMAL};
        graphics = new MgmGPU{};
        graphics->connect_to_window(window);

#if !defined(EMBED_BACKEND)
#if defined(__linux__)
        graphics->load_backend("shared/libbackend_OpenGL.so");
#elif defined (WIN32) || defined(_WIN32)
        graphics->load_backend("shared/backend_OpenGL.dll");
#endif
#endif

        auto& settings = graphics->settings();
        settings.clear.color = {0.1f, 0.2f, 0.3f, 1.0f};
        settings.viewport.top_left = {0, 0};
        settings.viewport.bottom_right = vec2i32{static_cast<int>(window->get_size().x()), static_cast<int>(window->get_size().y())};

        graphics->apply_settings(true);
        graphics->draw_list.emplace_back(MgmGPU::DrawCall{
            .type = MgmGPU::DrawCall::Type::CLEAR
        });

        
        ImGui_ImplMgmGFX_Init(*graphics);

        initialized = true;
    }

    void MagmaEngine::run() {
        if (!initialized) return;
        auto start = std::chrono::high_resolution_clock::now();
        float delta = 1.0f;

#if defined(ENABLE_EDITOR)
        if (!systems().try_get<Editor>())
            for (const auto& [id, sys] : systems().systems)
                sys->on_begin_play();
        else
            systems().get<Editor>().on_begin_play();
#else
        for (const auto& [id, sys] : systems.systems)
            sys->init();
#endif

        while (!window->should_close()) {
            constexpr auto delta_avg_calc_ratio = 0.05f;
            const auto now = std::chrono::high_resolution_clock::now();
            const auto chrono_delta = std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
            start = now;
            delta = delta * (1.0f - delta_avg_calc_ratio) + (float)chrono_delta * 0.000001f * delta_avg_calc_ratio;

            window->update();
            ImGui_ImplMgmGFX_ProcessInput(*window);
            ImGui_ImplMgmGFX_NewFrame();
            ImGui::NewFrame();

            ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

            for (const auto& [id, sys] : systems().systems) {
                sys->update(delta);
            }

            ImGui::SetNextWindowPos({0, 0});
            ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize
                | ImGuiWindowFlags_NoResize
                | ImGuiWindowFlags_NoMove
                | ImGuiWindowFlags_NoCollapse
                | ImGuiWindowFlags_NoTitleBar
                | ImGuiWindowFlags_NoBackground
                | ImGuiWindowFlags_NoSavedSettings
                | ImGuiWindowFlags_NoInputs
            );
            ImGui::Text("%.2f", 1.0f / delta);
            ImGui::End();

            ImGui::EndFrame();
            ImGui::Render();

            graphics->draw();
            ImGui_ImplMgmGFX_RenderDrawData(ImGui::GetDrawData());
            graphics->present();
        }

#if !defined(ENABLE_EDITOR)
        for (const auto& [id, sys] : systems().systems)
            sys->on_end_play();
#endif
    }

    MagmaEngine::~MagmaEngine() {
        if (this != instance) return;

        if (!initialized) return;
        delete graphics;
        delete window;
        instance = nullptr;
    }
}
