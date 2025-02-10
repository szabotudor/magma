#include "engine.hpp"
#include "built-in_components/renderable.hpp"
#include "ecs.hpp"
#include "imgui.h"
#include "imgui_impl_mgmgpu.h"
#include "mgmath.hpp"
#include "systems/input.hpp"
#include "logging.hpp"
#include "mgmgpu.hpp"
#include "mgmwin.hpp"
#include "systems/notifications.hpp"
#include "file.hpp"
#include "systems/resources.hpp"
#include "systems/renderer.hpp"

#if defined(ENABLE_EDITOR)
#include "systems/editor.hpp"
#endif

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>


namespace mgm {
    struct MagmaEngine::Data {
        ExtractedDrawData* imgui_draw_data = nullptr;

        FileIO* file_io = nullptr;
        MgmWindow* window = nullptr;
        MgmGPU* graphics = nullptr;
        MgmGPU::Settings graphics_settings{};
        std::vector<MgmGPU::DrawCall> basic_draw_list{};

        SystemManager* system_manager = nullptr;

        float current_dt = 0.0f;
    };

    MagmaEngine::Data* MagmaEngine::data = nullptr;

    void MagmaEngine::render_thread_function() {
        while (engine_running) {
            graphics_settings_mutex.lock();
            const auto settings = data->graphics_settings;
            graphics_settings_mutex.unlock();
            graphics().draw(data->basic_draw_list, settings);

            std::unique_lock lock{systems().mutex};
            for (auto& system : systems().systems)
                system.second->graphics_update();
            lock.unlock();

            imgui_mutex.lock();
            if (data->imgui_draw_data->is_set)
                ImGui_ImplMgmGFX_RenderDrawData(*data->imgui_draw_data, settings);
            imgui_mutex.unlock();

            graphics().present();
        }
    }

    FileIO& MagmaEngine::file_io() { return *data->file_io; }
    MgmWindow& MagmaEngine::window() { return *data->window; }
    ResourceManager& MagmaEngine::resource_manager() { return systems().get<ResourceManager>(); }
    Input& MagmaEngine::input() { return systems().get<Input>(); }
    Notifications& MagmaEngine::notifications() { return systems().get<Notifications>(); }
    MgmGPU& MagmaEngine::graphics() { return *data->graphics; }
    Renderer& MagmaEngine::renderer() { return systems().get<Renderer>(); }
#if defined(ENABLE_EDITOR)
    Editor& MagmaEngine::editor() {
        const auto e = systems().try_get<Editor>();
        if (e == nullptr) {
            Logging{"Engine"}.error("Engine was ran without an editor, give the \"--editor\" argument to enable it");
            throw std::runtime_error("Editor not initialized");
        }
        return *e;
    }
#endif
    EntityComponentSystem& MagmaEngine::ecs() { return systems().get<EntityComponentSystem>(); }

    SystemManager& MagmaEngine::systems() { return *data->system_manager; }

    MagmaEngine::MagmaEngine(const std::vector<std::string>& args) {
        if (data != nullptr)
            return;

        data = new Data{};
        Path::setup_project_dirs(
            FileIO::exe_dir().data,
            FileIO::exe_dir().data + "/assets",
            FileIO::exe_dir().data + "/data"
        );

        data->imgui_draw_data = new ExtractedDrawData{};

        data->system_manager = new SystemManager{};

        bool help_called = false;

        if (std::find(args.begin(), args.end(), "--help") != args.end()) {
            std::cout << "Usage: " << Path::project_dir.file_name() << " [options]\n"
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
        graphics().load_backend("exe://shared/libbackend_OpenGL.so");
#elif defined (WIN32) || defined(_WIN32)
        if (file_io().exists("exe://shared/backend_OpenGL.dll"))
            graphics().load_backend("exe://shared/backend_OpenGL.dll");
        else
            graphics().load_backend("exe://shared/libbackend_OpenGL.dll");
#endif
#endif

        data->graphics_settings.backend.clear.color = {0.1f, 0.2f, 0.3f, 1.0f};
        data->graphics_settings.backend.viewport.top_left = {0, 0};
        data->graphics_settings.backend.viewport.bottom_right = vec2i32{static_cast<int>(window().get_size().x), static_cast<int>(window().get_size().y)};

        data->basic_draw_list.emplace_back(MgmGPU::DrawCall{
            .type = MgmGPU::DrawCall::Type::CLEAR
        });


        const auto imgui_shader_source = file_io().read_text("resources://shaders/imgui.shader");

        ImGui_ImplMgmGFX_Init(graphics(), imgui_shader_source);

        systems().create<ResourceManager>();
        systems().create<Input>();
        systems().create<Notifications>();
        systems().create<EntityComponentSystem>();
        systems().create<Renderer>();

#if defined(ENABLE_EDITOR)
        if (std::find(args.begin(), args.end(), "--editor") != args.end())
            systems().create<Editor>();
        else
#endif
        // Will execute if the editor is not enabled, or if the editor is enabled and the argument "--editor" is not given
        {
        }

        ecs().enable_type_serialization<vec2f>("vec2f");
        ecs().enable_type_serialization<vec3f>("vec3f");
        ecs().enable_type_serialization<vec4f>("vec4f");
        ecs().enable_type_serialization<vec2d>("vec2d");
        ecs().enable_type_serialization<vec3d>("vec3d");
        ecs().enable_type_serialization<vec4d>("vec4d");
        ecs().enable_type_serialization<Transform>("Transform", true);

        ecs().enable_type_serialization<ResourceReference<Mesh>>("Mesh", true);
        resource_manager().asociate_resource_with_file_extension<Mesh>("obj");

        ecs().enable_type_serialization<ResourceReference<Shader>>("Shader", true);
        resource_manager().asociate_resource_with_file_extension<Shader>("shader");

        initialized = true;
    }

    void MagmaEngine::run() {
        if (!initialized) {
            Logging{"Engine"}.error("Do not call \"run\" on a secondary instance of MagmaEngine");
            return;
        }

        if (!file_io().exists(Path::assets_dir)) file_io().create_folder(Path::assets_dir);
        if (!file_io().exists(Path::game_data_dir)) file_io().create_folder(Path::game_data_dir);

        auto start = std::chrono::high_resolution_clock::now();

#if defined(ENABLE_EDITOR)
        if (!systems().try_get<Editor>())
            for (const auto& [id, sys] : systems().systems)
                sys->on_begin_play();
        else
            systems().get<Editor>().on_begin_play();
#else
        for (const auto& [id, sys] : systems.systems)
            sys->on_begin_play();
#endif

        engine_running = true;
        std::thread render_thread{&MagmaEngine::render_thread_function, this};

        auto window_size = window().get_size();

        while (!window().should_close()) {
            const auto now = std::chrono::high_resolution_clock::now();
            const auto chrono_delta = std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
            start = now;
            const float delta = (float)chrono_delta * 0.000001f;
            data->current_dt = delta;

            window().update();

            ImGui::GetIO().DeltaTime = delta;
            ImGui_ImplMgmGFX_ProcessInput(window());
            ImGui_ImplMgmGFX_NewFrame();
            ImGui::NewFrame();

            ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_PassthruCentralNode);


#if defined(ENABLE_EDITOR)
            if (const auto editor_ptr = systems().try_get<Editor>())
                editor_ptr->update(delta);
            else {
                for (const auto& [id, sys] : systems().systems)
                    sys->update(delta);
            }
#else
            for (const auto& [id, sys] : systems().systems)
                sys->update(delta);
#endif

            ImGui::EndFrame();
            ImGui::Render();

            imgui_mutex.lock();
            if (data->imgui_draw_data->is_set)
                data->imgui_draw_data->clear();

            graphics_settings_mutex.lock();
            const auto viewport = data->graphics_settings.backend.viewport;
            graphics_settings_mutex.unlock();
            extract_draw_data(ImGui::GetDrawData(), *data->imgui_draw_data, viewport);

            imgui_mutex.unlock();

            if (window().get_size() != window_size) {
                window_size = window().get_size();
                graphics_settings_mutex.lock();
                data->graphics_settings.backend.viewport.top_left = {0, 0};
                data->graphics_settings.backend.viewport.bottom_right = vec2i32{static_cast<int>(window_size.x), static_cast<int>(window_size.y)};
                graphics_settings_mutex.unlock();
            }

            // Sleep the thread for a bit to not stress the CPU too much
            // TODO: Do this better, because this is just a hack
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        engine_running = false;
        render_thread.join();

#if defined(ENABLE_EDITOR)
        if (!systems().try_get<Editor>())
#endif
            for (const auto& [id, sys] : systems().systems)
                sys->on_end_play();
        else
            systems().destroy<Editor>();
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
