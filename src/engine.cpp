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
#include <thread>


namespace mgm {
    MagmaEngine* MagmaEngine::instance = nullptr;

    void MagmaEngine::render_thread_function() {
        while (engine_running) {
            m_graphics->draw();

            imgui_mutex.lock();
            if (m_imgui_draw_data->is_set)
                ImGui_ImplMgmGFX_RenderDrawData(*m_imgui_draw_data);
            imgui_mutex.unlock();

            m_graphics->present();
        }
    }

    MagmaEngine::MagmaEngine(const std::vector<std::string>& args) {
        if (!instance) {
            instance = this;
        } else {
            m_imgui_draw_data = instance->m_imgui_draw_data;
            m_file_io = instance->m_file_io;
            m_window = instance->m_window;
            m_graphics = instance->m_graphics;
            m_system_manager = instance->m_system_manager;
            initialized = true;
            return;
        }

        m_imgui_draw_data = new ExtractedDrawData{};

        m_system_manager = new SystemManager{};

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


        m_window = new MgmWindow{"Magma", vec2u32{800, 600}, MgmWindow::Mode::NORMAL};
        m_graphics = new MgmGPU{};
        m_graphics->connect_to_window(m_window);

#if !defined(EMBED_BACKEND)
#if defined(__linux__)
        m_graphics->load_backend("shared/libbackend_OpenGL.so");
#elif defined (WIN32) || defined(_WIN32)
        m_graphics->load_backend("shared/backend_OpenGL.dll");
#endif
#endif

        auto& settings = m_graphics->settings();
        settings.clear.color = {0.1f, 0.2f, 0.3f, 1.0f};
        settings.viewport.top_left = {0, 0};
        settings.viewport.bottom_right = vec2i32{static_cast<int>(m_window->get_size().x()), static_cast<int>(m_window->get_size().y())};

        m_graphics->apply_settings(true);
        m_graphics->draw_list.emplace_back(MgmGPU::DrawCall{
            .type = MgmGPU::DrawCall::Type::CLEAR
        });

        
        ImGui_ImplMgmGFX_Init(*m_graphics);

        initialized = true;
    }

    void MagmaEngine::run() {
        if (!initialized) return;

        if (this != instance) {
            Logging{"Engine"}.error("Do not call \"run\" on a secondary instance of MagmaEngine");
            return;
        }
        auto start = std::chrono::high_resolution_clock::now();
        float avg_delta = 1.0f;

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

        engine_running = true;
        std::thread render_thread{&MagmaEngine::render_thread_function, this};

        while (!m_window->should_close()) {
            constexpr auto delta_avg_calc_ratio = 0.00001f;
            const auto now = std::chrono::high_resolution_clock::now();
            const auto chrono_delta = std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
            start = now;
            const float delta = (float)chrono_delta * 0.000001f;
            avg_delta = avg_delta * (1.0f - delta_avg_calc_ratio) + (float)chrono_delta * 0.000001f * delta_avg_calc_ratio;

            m_window->update();

            ImGui_ImplMgmGFX_ProcessInput(*m_window);
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
                | ImGuiWindowFlags_NoDocking
            );
            ImGui::Text("%i FPS", static_cast<int>(1.0f / avg_delta));
            ImGui::End();

            ImGui::EndFrame();
            ImGui::Render();

            imgui_mutex.lock();
            if (m_imgui_draw_data->is_set)
                m_imgui_draw_data->clear();
            extract_draw_data(ImGui::GetDrawData(), *m_imgui_draw_data, *m_graphics);
            imgui_mutex.unlock();
        }

        engine_running = false;
        render_thread.join();

#if !defined(ENABLE_EDITOR)
        for (const auto& [id, sys] : systems().systems)
            sys->on_end_play();
#endif
    }

    MagmaEngine::~MagmaEngine() {
        if (this != instance) return;

        if (!initialized) return;
        delete m_file_io;
        delete m_graphics;
        delete m_window;
        delete m_system_manager;
        delete m_imgui_draw_data;
        instance = nullptr;
    }
}
