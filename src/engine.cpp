#include "engine.hpp"
#include "backend_settings.hpp"
#include "editor.hpp"
#include "imgui.h"
#include "imgui_impl_mgmgpu.h"
#include "input.hpp"
#include "logging.hpp"
#include "mgmgpu.hpp"
#include "mgmwin.hpp"
#include "notifications.hpp"
#include "engine_manager.hpp"

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

    Input& MagmaEngine::input() { return systems().get<Input>(); }
    Notifications& MagmaEngine::notifications() { return systems().get<Notifications>(); }
    EngineManager& MagmaEngine::engine_manager() { return systems().get<EngineManager>(); }

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

        if (std::find(args.begin(), args.end(), "--help") != args.end()) {
            std::cout << "Usage: " << Path::exe_dir.file_name() << " [options]\n"
                << "Options:\n"
                << "\t--help\t\tShow this help message\n"
#if defined(ENABLE_EDITOR)
                << "\t--editor\tStart the editor\n"
#endif
            ;
            help_called = true;
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

        systems().create<Input>();
        systems().create<Notifications>();
        systems().create<EngineManager>();

#if defined(ENABLE_EDITOR)
        if (std::find(args.begin(), args.end(), "--editor") != args.end())
            systems().create<Editor>();
#endif

        initialized = true;
    }

    void MagmaEngine::run() {
        if (!initialized) return;

        if (!file_io().exists(Path::assets)) file_io().create_folder(Path::assets);
        if (!file_io().exists(Path::game_data)) file_io().create_folder(Path::game_data);
        if (!file_io().exists(Path::temp)) file_io().create_folder(Path::temp);

        if (this != instance) {
            Logging{"Engine"}.error("Do not call \"run\" on a secondary instance of MagmaEngine");
            return;
        }
        auto start = std::chrono::high_resolution_clock::now();

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
            const auto now = std::chrono::high_resolution_clock::now();
            const auto chrono_delta = std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
            start = now;
            const float delta = (float)chrono_delta * 0.000001f;
            instance->current_dt = delta;

            const auto window_old_size = m_window->get_size();

            m_window->update();

            const auto window_size = m_window->get_size();

            if (window_size != window_old_size) {
                m_graphics->lock_mutex();
                m_graphics->settings().viewport.top_left = {0, 0};
                m_graphics->settings().viewport.bottom_right = vec2i32{static_cast<int>(window_size.x()), static_cast<int>(window_size.y())};
                m_graphics->unlock_mutex();
                m_graphics->apply_settings(true);
            }

            ImGui::GetIO().DeltaTime = delta;
            ImGui_ImplMgmGFX_ProcessInput(*m_window);
            ImGui_ImplMgmGFX_NewFrame();
            ImGui::NewFrame();

            ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_PassthruCentralNode);
            
            if (engine_manager().font)
                ImGui::PushFont(engine_manager().font);

#if defined(ENABLE_EDITOR)
            if (systems().try_get<Editor>())
                systems().get<Editor>().update(delta);
            else {
                for (const auto& [id, sys] : systems().systems)
                    sys->update(delta);
            }
#else
            for (const auto& [id, sys] : systems().systems)
                sys->update(delta);
#endif

            if (engine_manager().font)
                ImGui::PopFont();

            ImGui::EndFrame();
            ImGui::Render();

            imgui_mutex.lock();
            if (m_imgui_draw_data->is_set)
                m_imgui_draw_data->clear();

            m_graphics->lock_mutex();
            const auto viewport = m_graphics->settings().viewport;
            m_graphics->unlock_mutex();
            extract_draw_data(ImGui::GetDrawData(), *m_imgui_draw_data, viewport);

            imgui_mutex.unlock();
        }

        engine_running = false;
        render_thread.join();

#if defined(ENABLE_EDITOR)
        if (!systems().try_get<Editor>())
#endif
            for (const auto& [id, sys] : systems().systems)
                sys->on_end_play();
    }

    float MagmaEngine::delta_time() const {
        return instance->current_dt;
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
