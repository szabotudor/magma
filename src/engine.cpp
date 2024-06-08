#include "engine.hpp"
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
#include <thread>


namespace mgm {
    struct MagmaEngine::Data {
        ExtractedDrawData* imgui_draw_data = nullptr;

        FileIO* file_io = nullptr;
        MgmWindow* window = nullptr;
        MgmGPU* graphics = nullptr;
        Settings graphics_settings{};
        std::vector<MgmGPU::DrawCall> draw_list{};

        SystemManager* system_manager = nullptr;

        float current_dt = 0.0f;
    };

    MagmaEngine::Data* MagmaEngine::data = nullptr;

    void MagmaEngine::render_thread_function() {
        while (engine_running) {
            graphics_settings_mutex.lock();
            const auto settings = data->graphics_settings;
            graphics_settings_mutex.unlock();
            graphics().draw(data->draw_list, settings);

            imgui_mutex.lock();
            if (data->imgui_draw_data->is_set)
                ImGui_ImplMgmGFX_RenderDrawData(*data->imgui_draw_data);
            imgui_mutex.unlock();

            graphics().present();
        }
    }

    FileIO& MagmaEngine::file_io() { return *data->file_io; }
    MgmWindow& MagmaEngine::window() { return *data->window; }
    Input& MagmaEngine::input() { return systems().get<Input>(); }
    Notifications& MagmaEngine::notifications() { return systems().get<Notifications>(); }
    EngineManager& MagmaEngine::engine_manager() { return systems().get<EngineManager>(); }
    MgmGPU& MagmaEngine::graphics() { return *data->graphics; }
    SystemManager& MagmaEngine::systems() { return *data->system_manager; }

    MagmaEngine::MagmaEngine(const std::vector<std::string>& args) {
        if (!data)
            data = new Data{};
        else
            return;

        data->imgui_draw_data = new ExtractedDrawData{};

        data->system_manager = new SystemManager{};

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


        data->window = new MgmWindow{"Magma", vec2u32{800, 600}, MgmWindow::Mode::NORMAL};
        data->graphics = new MgmGPU{};
        graphics().connect_to_window(&window());

#if !defined(EMBED_BACKEND)
#if defined(__linux__)
        graphics().load_backend("shared/libbackend_OpenGL.so");
#elif defined (WIN32) || defined(_WIN32)
        m_graphics->load_backend("shared/backend_OpenGL.dll");
#endif
#endif

        data->graphics_settings.clear.color = {0.1f, 0.2f, 0.3f, 1.0f};
        data->graphics_settings.viewport.top_left = {0, 0};
        data->graphics_settings.viewport.bottom_right = vec2i32{static_cast<int>(window().get_size().x()), static_cast<int>(window().get_size().y())};

        data->draw_list.emplace_back(MgmGPU::DrawCall{
            .type = MgmGPU::DrawCall::Type::CLEAR
        });

        
        ImGui_ImplMgmGFX_Init(graphics());

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
        if (!initialized) {
            Logging{"Engine"}.error("Do not call \"run\" on a secondary instance of MagmaEngine");
            return;
        }

        if (!file_io().exists(Path::assets)) file_io().create_folder(Path::assets);
        if (!file_io().exists(Path::game_data)) file_io().create_folder(Path::game_data);
        if (!file_io().exists(Path::temp)) file_io().create_folder(Path::temp);

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

        while (!window().should_close()) {
            const auto now = std::chrono::high_resolution_clock::now();
            const auto chrono_delta = std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
            start = now;
            const float delta = (float)chrono_delta * 0.000001f;
            data->current_dt = delta;

            const auto window_old_size = window().get_size();

            window().update();

            const auto window_size = window().get_size();

            if (window_size != window_old_size) {
                graphics_settings_mutex.lock();
                data->graphics_settings.viewport.top_left = {0, 0};
                data->graphics_settings.viewport.bottom_right = vec2i32{static_cast<int>(window_size.x()), static_cast<int>(window_size.y())};
                graphics_settings_mutex.unlock();
            }

            ImGui::GetIO().DeltaTime = delta;
            ImGui_ImplMgmGFX_ProcessInput(window());
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
            if (data->imgui_draw_data->is_set)
                data->imgui_draw_data->clear();

            graphics_settings_mutex.lock();
            const auto viewport = data->graphics_settings.viewport;
            graphics_settings_mutex.unlock();
            extract_draw_data(ImGui::GetDrawData(), *data->imgui_draw_data, viewport);

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
        return data->current_dt;
    }

    MagmaEngine::~MagmaEngine() {
        if (!initialized) return;

        if (!initialized) return;
        delete data->file_io;
        delete data->graphics;
        delete data->window;
        delete data->system_manager;
        delete data->imgui_draw_data;
        delete data;
        data = nullptr;
    }
}
