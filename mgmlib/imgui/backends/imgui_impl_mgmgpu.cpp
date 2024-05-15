#include "backend_settings.hpp"
#include "imgui_impl_mgmgpu.h"
#include "imgui.h"
#include "logging.hpp"
#include "mgmath.hpp"
#include "mgmgpu.hpp"
#include "mgmwin.hpp"
#include <cstdint>
#include <stdexcept>



struct ImGui_BackendData {
    mgm::MgmGPU* backend = nullptr;
    mgm::MgmGPU::TextureHandle font_atlas{};
    mgm::MgmGPU::ShaderHandle font_atlas_shader{};
    mgm::Logging log{"ImGui MgmGFX Backend"};
};
ImGui_BackendData* get_backend_data() {
    if (ImGui::GetCurrentContext() != nullptr)
        return (ImGui_BackendData*)ImGui::GetIO().BackendRendererUserData;
    throw std::runtime_error("ImGui backend not initialized");
}

bool ImGui_ImplMgmGFX_Init(mgm::MgmGPU &backend) {
    const auto ctx = ImGui::CreateContext();
    ImGui::SetCurrentContext(ctx);

    auto& io = ImGui::GetIO();
    if (io.BackendRendererUserData != nullptr)
        throw std::runtime_error("ImGui backend already initialized");

    const auto viewport = backend.settings().viewport;
    io.DisplaySize = {static_cast<float>(viewport.bottom_right.x()), static_cast<float>(viewport.bottom_right.y())};

    uint8_t* tex_data = nullptr;
    mgm::vec2i32 size{};
    io.Fonts->GetTexDataAsRGBA32(&tex_data, &size.x(), &size.y());
    const auto texture_info = mgm::TextureCreateInfo{4, 1, 2, size, tex_data};
    const auto fonts_texture = backend.create_texture(texture_info);
    io.Fonts->SetTexID(reinterpret_cast<void*>(static_cast<intptr_t>(fonts_texture.id)));
    
    mgm::ShaderCreateInfo fonts_shader_info{};
    fonts_shader_info.shader_sources.emplace_back(mgm::ShaderCreateInfo::SingleShaderInfo{
        mgm::ShaderCreateInfo::SingleShaderInfo::Type::VERTEX,
        "#version 460 core\n"
        "layout (location = 0) in vec3 Vert;\n"
        "layout (location = 1) in vec4 VertColor;\n"
        "layout (location = 2) in vec2 TexCoords;\n"
        "uniform mat4 Proj;\n"
        "out vec2 Frag_TexCoords;\n"
        "out vec4 Frag_VertColor;\n"
        "void main() {\n"
        "   Frag_TexCoords = TexCoords;\n"
        "   Frag_VertColor = VertColor;\n"
        "   gl_Position = Proj * vec4(Vert, 1.0f);\n"
        "}\n"
    });
    fonts_shader_info.shader_sources.emplace_back(mgm::ShaderCreateInfo::SingleShaderInfo{
        mgm::ShaderCreateInfo::SingleShaderInfo::Type::FRAGMENT,
        "#version 460 core\n"
        "in vec2 Frag_TexCoords;\n"
        "in vec4 Frag_VertColor;\n"
        "uniform sampler2D Texture;\n"
        "out vec4 FragColor;"
        "void main() {\n"
        "   FragColor = Frag_VertColor * texture(Texture, Frag_TexCoords);\n"
        "}\n"
    });
    
    const auto fonts_shader = backend.create_shader(fonts_shader_info);

    io.BackendRendererUserData = new ImGui_BackendData{
        &backend,
        fonts_texture,
        fonts_shader
    };
    get_backend_data()->log.log("Initialized ImGui backend");

    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigDockingWithShift = false;
    io.ConfigDockingAlwaysTabBar = false;
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    return true;
}

void ImGui_ImplMgmGFX_Shutdown() {
    auto* data = get_backend_data();
    data->backend->destroy_texture(data->font_atlas);
    data->backend->destroy_shader(data->font_atlas_shader);
    data->log.log("Shutdown ImGui MgmGFX backend");
    delete get_backend_data();
}

void ImGui_ImplMgmGFX_NewFrame() {
    auto* data = get_backend_data();
    auto& backend = *data->backend;
    auto& io = ImGui::GetIO();

    if (!io.Fonts->IsBuilt()) {
        uint8_t* tex_data = nullptr;
        mgm::vec2i32 size{};
        io.Fonts->GetTexDataAsRGBA32(&tex_data, &size.x(), &size.y());
        const auto texture_info = mgm::TextureCreateInfo{4, 1, 2, size, tex_data};
        backend.destroy_texture(data->font_atlas);
        data->font_atlas = backend.create_texture(texture_info);
        io.Fonts->SetTexID(reinterpret_cast<void*>(static_cast<intptr_t>(data->font_atlas.id)));
    }
}

void extract_draw_data(ImDrawData* draw_data, ExtractedDrawData& out, const mgm::MgmGPU& backend) {
    float L = draw_data->DisplayPos.x;
    float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    float T = draw_data->DisplayPos.y;
    float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;

    out.proj = {
        2.0f/(R-L),   0.0f,         0.0f,   0.0f,
        0.0f,         2.0f/(T-B),   0.0f,   0.0f,
        0.0f,         0.0f,        -1.0f,   0.0f,
        (R+L)/(L-R),  (T+B)/(B-T),  0.0f,   1.0f
    };

    for (int i = 0; i < draw_data->CmdListsCount; i++) {
        const auto* cmd_list = draw_data->CmdLists[i];

        auto& cmd = out.cmds.emplace_back();

        cmd.verts.reserve(cmd_list->VtxBuffer.Size);
        cmd.coords.reserve(cmd_list->VtxBuffer.Size);
        cmd.colors.reserve(cmd_list->VtxBuffer.Size);
        for (const auto& v : cmd_list->VtxBuffer) {
            cmd.verts.emplace_back(v.pos.x, v.pos.y, 0.0f);
            cmd.coords.emplace_back(v.uv.x, v.uv.y);
            cmd.colors.emplace_back(
                ((float)((uint8_t*)&v.col)[0] * 3.906250e-03f),
                ((float)((uint8_t*)&v.col)[1] * 3.906250e-03f),
                ((float)((uint8_t*)&v.col)[2] * 3.906250e-03f),
                ((float)((uint8_t*)&v.col)[3] * 3.906250e-03f)
            );
        }

        for (const auto& ind : cmd_list->IdxBuffer)
            cmd.indices.emplace_back((uint32_t)ind);

        for (int j = 0; j < cmd_list->CmdBuffer.Size; j++) {
            auto& cmd_data = cmd.cmd_data.emplace_back();
            const auto* im_cmd = &cmd_list->CmdBuffer[j];

            if (im_cmd->UserCallback) {
                im_cmd->UserCallback(cmd_list, im_cmd);
            }
            else {
                const auto clip_off = draw_data->DisplayPos;
                const auto clip_scale = draw_data->FramebufferScale;
                
                ImVec2 clip_min{
                    (im_cmd->ClipRect.x - clip_off.x) * clip_scale.x,
                    (im_cmd->ClipRect.y - clip_off.y) * clip_scale.y
                };
                ImVec2 clip_max{
                    (im_cmd->ClipRect.z - clip_off.x) * clip_scale.x,
                    (im_cmd->ClipRect.w - clip_off.y) * clip_scale.y
                };
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                    continue;

                cmd_data.scissor.top_left = {static_cast<int>(clip_min.x), backend.settings().viewport.bottom_right.y() - static_cast<int>(clip_max.y)};
                cmd_data.scissor.bottom_right = {static_cast<int>(clip_max.x), backend.settings().viewport.bottom_right.y() - static_cast<int>(clip_min.y)};

                cmd_data.idx_offset = im_cmd->IdxOffset;
                cmd_data.elem_count = im_cmd->ElemCount;
            }
        }
    }
    out.is_set = true;
}

void ImGui_ImplMgmGFX_RenderDrawData(ExtractedDrawData& draw_data) {
    auto* data = get_backend_data();
    auto& backend = *data->backend;
    backend.stash();

    backend.settings().blending.enabled = true;
    backend.settings().blending.color_equation = mgm::Settings::Blending::Equation::ADD;
    backend.settings().blending.alpha_equation = mgm::Settings::Blending::Equation::ADD;
    backend.settings().blending.src_color_factor = mgm::Settings::Blending::Factor::SRC_ALPHA;
    backend.settings().blending.dst_color_factor = mgm::Settings::Blending::Factor::ONE_MINUS_SRC_ALPHA;
    backend.settings().blending.src_alpha_factor = mgm::Settings::Blending::Factor::ONE;
    backend.settings().blending.dst_alpha_factor = mgm::Settings::Blending::Factor::ONE;

    for (const auto& cmd : draw_data.cmds) {
        const auto mesh_verts = backend.create_buffer({mgm::BufferCreateInfo::Type::RAW, cmd.verts.data(), cmd.verts.size()});
        const auto mesh_colors = backend.create_buffer({mgm::BufferCreateInfo::Type::RAW, cmd.colors.data(), cmd.colors.size()});
        const auto mesh_coords = backend.create_buffer({mgm::BufferCreateInfo::Type::RAW, cmd.coords.data(), cmd.coords.size()});

        std::vector<mgm::MgmGPU::BufferHandle> leftover_buffers{};

        for (const auto& cmd_data : cmd.cmd_data) {
            auto settings = backend.settings();
            settings.scissor = cmd_data.scissor;
            settings.scissor.enabled = true;
            backend.draw_list.emplace_back(mgm::MgmGPU::DrawCall{
                .type = mgm::MgmGPU::DrawCall::Type::SETTINGS_CHANGE,
                .parameters = {
                    {"settings", settings}
                }
            });

            const auto mesh_indices = backend.create_buffer({mgm::BufferCreateInfo::Type::INDEX, cmd.indices.data() + cmd_data.idx_offset, cmd_data.elem_count});
            leftover_buffers.emplace_back(mesh_indices);
            const auto mesh = backend.create_buffers_object({mesh_verts, mesh_colors, mesh_coords, mesh_indices});

            backend.draw_list.emplace_back(mgm::MgmGPU::DrawCall{
                .type = mgm::MgmGPU::DrawCall::Type::DRAW,
                .shader = data->font_atlas_shader,
                .buffers_object = mesh,
                .textures = {data->font_atlas},
                .parameters = {
                    {"Proj", draw_data.proj}
                }
            });
        }
        backend.draw();
        for (const auto& buf : leftover_buffers)
            backend.destroy_buffer(buf);
        for (const auto& draw_call : backend.draw_list)
            backend.destroy_buffers_object(draw_call.buffers_object);

        backend.destroy_buffer(mesh_verts);
        backend.destroy_buffer(mesh_coords);
        backend.destroy_buffer(mesh_colors);
        backend.draw_list.clear();
    }

    backend.pop_stash();
}

void ImGui_ImplMgmGFX_RenderDrawData(ImDrawData *draw_data) {
    auto* data = get_backend_data();
    auto& backend = *data->backend;
    backend.stash();

    backend.settings().blending.enabled = true;
    backend.settings().blending.color_equation = mgm::Settings::Blending::Equation::ADD;
    backend.settings().blending.alpha_equation = mgm::Settings::Blending::Equation::ADD;
    backend.settings().blending.src_color_factor = mgm::Settings::Blending::Factor::SRC_ALPHA;
    backend.settings().blending.dst_color_factor = mgm::Settings::Blending::Factor::ONE_MINUS_SRC_ALPHA;
    backend.settings().blending.src_alpha_factor = mgm::Settings::Blending::Factor::ONE;
    backend.settings().blending.dst_alpha_factor = mgm::Settings::Blending::Factor::ONE;

    mgm::mat4f proj{};
    {
        float L = draw_data->DisplayPos.x;
        float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
        float T = draw_data->DisplayPos.y;
        float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;

        proj = {
            2.0f/(R-L),   0.0f,         0.0f,   0.0f,
            0.0f,         2.0f/(T-B),   0.0f,   0.0f,
            0.0f,         0.0f,        -1.0f,   0.0f,
            (R+L)/(L-R),  (T+B)/(B-T),  0.0f,   1.0f
        };
    }

    for (int i = 0; i < draw_data->CmdListsCount; i++) {
        const auto* cmd_list = draw_data->CmdLists[i];

        std::vector<mgm::vec3f> verts{};
        std::vector<mgm::vec2f> coords{};
        std::vector<mgm::vec4f> colors{};
        verts.reserve(cmd_list->VtxBuffer.Size);
        coords.reserve(cmd_list->VtxBuffer.Size);
        colors.reserve(cmd_list->VtxBuffer.Size);
        for (const auto& v : cmd_list->VtxBuffer) {
            verts.emplace_back(v.pos.x, v.pos.y, 0.0f);
            coords.emplace_back(v.uv.x, v.uv.y);
            colors.emplace_back(
                ((float)((uint8_t*)&v.col)[0] * 3.906250e-03f),
                ((float)((uint8_t*)&v.col)[1] * 3.906250e-03f),
                ((float)((uint8_t*)&v.col)[2] * 3.906250e-03f),
                ((float)((uint8_t*)&v.col)[3] * 3.906250e-03f)
            );
        }

        std::vector<uint32_t> indices{};
        indices.reserve(cmd_list->IdxBuffer.Size);
        for (const auto& ind : cmd_list->IdxBuffer)
            indices.emplace_back((uint32_t)ind);

        const auto mesh_verts = backend.create_buffer({mgm::BufferCreateInfo::Type::RAW, verts.data(), verts.size()});
        const auto mesh_colors = backend.create_buffer({mgm::BufferCreateInfo::Type::RAW, colors.data(), colors.size()});
        const auto mesh_coords = backend.create_buffer({mgm::BufferCreateInfo::Type::RAW, coords.data(), coords.size()});

        std::vector<mgm::MgmGPU::BufferHandle> leftover_buffers{};

        for (int j = 0; j < cmd_list->CmdBuffer.Size; j++) {
            const auto* cmd = &cmd_list->CmdBuffer[j];

            if (cmd->UserCallback) {
                cmd->UserCallback(cmd_list, cmd);
            }
            else {
                const auto clip_off = draw_data->DisplayPos;
                const auto clip_scale = draw_data->FramebufferScale;
                
                ImVec2 clip_min{
                    (cmd->ClipRect.x - clip_off.x) * clip_scale.x,
                    (cmd->ClipRect.y - clip_off.y) * clip_scale.y
                };
                ImVec2 clip_max{
                    (cmd->ClipRect.z - clip_off.x) * clip_scale.x,
                    (cmd->ClipRect.w - clip_off.y) * clip_scale.y
                };
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                    continue;

                auto settings = backend.settings();
                settings.scissor.top_left = {static_cast<int>(clip_min.x), settings.viewport.bottom_right.y() - static_cast<int>(clip_max.y)};
                settings.scissor.bottom_right = {static_cast<int>(clip_max.x), settings.viewport.bottom_right.y() - static_cast<int>(clip_min.y)};
                settings.scissor.enabled = true;
                backend.draw_list.emplace_back(mgm::MgmGPU::DrawCall{
                    .type = mgm::MgmGPU::DrawCall::Type::SETTINGS_CHANGE,
                    .parameters = {
                        {"settings", settings}
                    }
                });

                const auto mesh_indices = backend.create_buffer({mgm::BufferCreateInfo::Type::INDEX, indices.data() + cmd->IdxOffset, cmd->ElemCount});
                leftover_buffers.emplace_back(mesh_indices);
                const auto mesh = backend.create_buffers_object({mesh_verts, mesh_colors, mesh_coords, mesh_indices});

                backend.draw_list.emplace_back(mgm::MgmGPU::DrawCall{
                    .type = mgm::MgmGPU::DrawCall::Type::DRAW,
                    .shader = data->font_atlas_shader,
                    .buffers_object = mesh,
                    .textures = {data->font_atlas},
                    .parameters = {
                        {"Proj", proj}
                    }
                });
            }
        }
        backend.draw();
        for (const auto& buf : leftover_buffers)
            backend.destroy_buffer(buf);
        for (const auto& draw_call : backend.draw_list)
            backend.destroy_buffers_object(draw_call.buffers_object);

        backend.destroy_buffer(mesh_verts);
        backend.destroy_buffer(mesh_coords);
        backend.destroy_buffer(mesh_colors);
        backend.draw_list.clear();
    }

    backend.pop_stash();
}

ImGuiMouseButton input_interface_to_imgui_mb(const mgm::MgmWindow::InputInterface ii) {
    switch (ii) {
        case mgm::MgmWindow::InputInterface::Mouse_LEFT: return ImGuiMouseButton_Left;
        case mgm::MgmWindow::InputInterface::Mouse_RIGHT: return ImGuiMouseButton_Right;
        case mgm::MgmWindow::InputInterface::Mouse_MIDDLE: return ImGuiMouseButton_Middle;
        default: return ImGuiMouseButton_COUNT;
    }
}

ImGuiKey input_interface_to_imgui_key(const mgm::MgmWindow::InputInterface ii) {
    switch (ii) {
        case mgm::MgmWindow::InputInterface::Key_A: return ImGuiKey_A;
        case mgm::MgmWindow::InputInterface::Key_B: return ImGuiKey_B;
        case mgm::MgmWindow::InputInterface::Key_C: return ImGuiKey_C;
        case mgm::MgmWindow::InputInterface::Key_D: return ImGuiKey_D;
        case mgm::MgmWindow::InputInterface::Key_E: return ImGuiKey_E;
        case mgm::MgmWindow::InputInterface::Key_F: return ImGuiKey_F;
        case mgm::MgmWindow::InputInterface::Key_G: return ImGuiKey_G;
        case mgm::MgmWindow::InputInterface::Key_H: return ImGuiKey_H;
        case mgm::MgmWindow::InputInterface::Key_I: return ImGuiKey_I;
        case mgm::MgmWindow::InputInterface::Key_J: return ImGuiKey_J;
        case mgm::MgmWindow::InputInterface::Key_K: return ImGuiKey_K;
        case mgm::MgmWindow::InputInterface::Key_L: return ImGuiKey_L;
        case mgm::MgmWindow::InputInterface::Key_M: return ImGuiKey_M;
        case mgm::MgmWindow::InputInterface::Key_N: return ImGuiKey_N;
        case mgm::MgmWindow::InputInterface::Key_O: return ImGuiKey_O;
        case mgm::MgmWindow::InputInterface::Key_P: return ImGuiKey_P;
        case mgm::MgmWindow::InputInterface::Key_Q: return ImGuiKey_Q;
        case mgm::MgmWindow::InputInterface::Key_R: return ImGuiKey_R;
        case mgm::MgmWindow::InputInterface::Key_S: return ImGuiKey_S;
        case mgm::MgmWindow::InputInterface::Key_T: return ImGuiKey_T;
        case mgm::MgmWindow::InputInterface::Key_U: return ImGuiKey_U;
        case mgm::MgmWindow::InputInterface::Key_V: return ImGuiKey_V;
        case mgm::MgmWindow::InputInterface::Key_W: return ImGuiKey_W;
        case mgm::MgmWindow::InputInterface::Key_X: return ImGuiKey_X;
        case mgm::MgmWindow::InputInterface::Key_Y: return ImGuiKey_Y;
        case mgm::MgmWindow::InputInterface::Key_Z: return ImGuiKey_Z;
        case mgm::MgmWindow::InputInterface::Key_0: return ImGuiKey_0;
        case mgm::MgmWindow::InputInterface::Key_1: return ImGuiKey_1;
        case mgm::MgmWindow::InputInterface::Key_2: return ImGuiKey_2;
        case mgm::MgmWindow::InputInterface::Key_3: return ImGuiKey_3;
        case mgm::MgmWindow::InputInterface::Key_4: return ImGuiKey_4;
        case mgm::MgmWindow::InputInterface::Key_5: return ImGuiKey_5;
        case mgm::MgmWindow::InputInterface::Key_6: return ImGuiKey_6;
        case mgm::MgmWindow::InputInterface::Key_7: return ImGuiKey_7;
        case mgm::MgmWindow::InputInterface::Key_8: return ImGuiKey_8;
        case mgm::MgmWindow::InputInterface::Key_9: return ImGuiKey_9;
        case mgm::MgmWindow::InputInterface::Key_META: return ImGuiKey_Space;
        case mgm::MgmWindow::InputInterface::Key_CAPSLOCK: return ImGuiKey_CapsLock;
        case mgm::MgmWindow::InputInterface::Key_NUMLOCK: return ImGuiKey_NumLock;
        case mgm::MgmWindow::InputInterface::Key_SCROLLLOCK: return ImGuiKey_ScrollLock;
        case mgm::MgmWindow::InputInterface::Key_SPACE: return ImGuiKey_Space;
        case mgm::MgmWindow::InputInterface::Key_ENTER: return ImGuiKey_Enter;
        case mgm::MgmWindow::InputInterface::Key_TAB: return ImGuiKey_Tab;
        case mgm::MgmWindow::InputInterface::Key_SHIFT: return ImGuiKey_LeftShift;
        case mgm::MgmWindow::InputInterface::Key_CTRL: return ImGuiKey_LeftCtrl;
        case mgm::MgmWindow::InputInterface::Key_ALT: return ImGuiKey_LeftAlt;
        case mgm::MgmWindow::InputInterface::Key_ESC: return ImGuiKey_Escape;
        case mgm::MgmWindow::InputInterface::Key_BACKSPACE: return ImGuiKey_Backspace;
        case mgm::MgmWindow::InputInterface::Key_DELETE: return ImGuiKey_Delete;
        case mgm::MgmWindow::InputInterface::Key_INSERT: return ImGuiKey_Insert;
        case mgm::MgmWindow::InputInterface::Key_HOME: return ImGuiKey_Home;
        case mgm::MgmWindow::InputInterface::Key_END: return ImGuiKey_End;
        case mgm::MgmWindow::InputInterface::Key_PAGEUP: return ImGuiKey_PageUp;
        case mgm::MgmWindow::InputInterface::Key_PAGEDOWN: return ImGuiKey_PageDown;
        case mgm::MgmWindow::InputInterface::Key_ARROW_UP: return ImGuiKey_UpArrow;
        case mgm::MgmWindow::InputInterface::Key_ARROW_DOWN: return ImGuiKey_DownArrow;
        case mgm::MgmWindow::InputInterface::Key_ARROW_LEFT: return ImGuiKey_LeftArrow;
        case mgm::MgmWindow::InputInterface::Key_ARROW_RIGHT: return ImGuiKey_RightArrow;
        case mgm::MgmWindow::InputInterface::Key_F1: return ImGuiKey_F1;
        case mgm::MgmWindow::InputInterface::Key_F2: return ImGuiKey_F2;
        case mgm::MgmWindow::InputInterface::Key_F3: return ImGuiKey_F3;
        case mgm::MgmWindow::InputInterface::Key_F4: return ImGuiKey_F4;
        case mgm::MgmWindow::InputInterface::Key_F5: return ImGuiKey_F5;
        case mgm::MgmWindow::InputInterface::Key_F6: return ImGuiKey_F6;
        case mgm::MgmWindow::InputInterface::Key_F7: return ImGuiKey_F7;
        case mgm::MgmWindow::InputInterface::Key_F8: return ImGuiKey_F8;
        case mgm::MgmWindow::InputInterface::Key_F9: return ImGuiKey_F9;
        case mgm::MgmWindow::InputInterface::Key_F10: return ImGuiKey_F10;
        case mgm::MgmWindow::InputInterface::Key_F11: return ImGuiKey_F11;
        case mgm::MgmWindow::InputInterface::Key_F12: return ImGuiKey_F12;
        case mgm::MgmWindow::InputInterface::Key_MINUS: return ImGuiKey_Minus;
        case mgm::MgmWindow::InputInterface::Key_EQUAL: return ImGuiKey_Equal;
        case mgm::MgmWindow::InputInterface::Key_COMMA: return ImGuiKey_Comma;
        case mgm::MgmWindow::InputInterface::Key_PERIOD: return ImGuiKey_Period;
        case mgm::MgmWindow::InputInterface::Key_SEMICOLON: return ImGuiKey_Semicolon;
        case mgm::MgmWindow::InputInterface::Key_APOSTROPHE: return ImGuiKey_Apostrophe;
        case mgm::MgmWindow::InputInterface::Key_OPEN_BRACKET: return ImGuiKey_LeftBracket;
        case mgm::MgmWindow::InputInterface::Key_CLOSE_BRACKET: return ImGuiKey_RightBracket;
        case mgm::MgmWindow::InputInterface::Key_OPEN_CURLY_BRACKET: return ImGuiKey_LeftBracket;
        case mgm::MgmWindow::InputInterface::Key_CLOSE_CURLY_BRACKET: return ImGuiKey_RightBracket;
        case mgm::MgmWindow::InputInterface::Key_BACKSLASH: return ImGuiKey_Backslash;
        case mgm::MgmWindow::InputInterface::Key_FORWARD_SLASH: return ImGuiKey_Slash;
        //case mgm::MgmWindow::InputInterface::Key_COLON: return ImGuiKey_;
        //case mgm::MgmWindow::InputInterface::Key_QUOTE: return ImGuiKey_;
        //case mgm::MgmWindow::InputInterface::Key_QUESTION_MARK: return ImGuiKey_;
        //case mgm::MgmWindow::InputInterface::Key_EXCLAMATION_MARK: return ImGuiKey_;
        //case mgm::MgmWindow::InputInterface::Key_AT: return ImGuiKey_;
        //case mgm::MgmWindow::InputInterface::Key_HASH: return ImGuiKey_;
        //case mgm::MgmWindow::InputInterface::Key_DOLLAR: return ImGuiKey_;
        //case mgm::MgmWindow::InputInterface::Key_PERCENT: return ImGuiKey_;
        //case mgm::MgmWindow::InputInterface::Key_CARET: return ImGuiKey_;
        //case mgm::MgmWindow::InputInterface::Key_LESS: return ImGuiKey_;
        //case mgm::MgmWindow::InputInterface::Key_GREATER: return ImGuiKey_;
        //case mgm::MgmWindow::InputInterface::Key_AMPERSAND: return ImGuiKey_;
        //case mgm::MgmWindow::InputInterface::Key_OPEN_PARENTHESIS: return ImGuiKey_;
        //case mgm::MgmWindow::InputInterface::Key_CLOSE_PARENTHESIS: return ImGuiKey_;
        //case mgm::MgmWindow::InputInterface::Key_UNDERSCORE: return ImGuiKey_;
        //case mgm::MgmWindow::InputInterface::Key_GRAVE: return ImGuiKey_;
        //case mgm::MgmWindow::InputInterface::Key_TILDE: return ImGuiKey_;
        //case mgm::MgmWindow::InputInterface::Key_VERTICAL_LINE: return ImGuiKey_;
        default: return ImGuiKey_None;
    }
}

void ImGui_ImplMgmGFX_ProcessInput(mgm::MgmWindow &window) {
    auto& io = ImGui::GetIO();
    float mouse_pos_x{};
    bool mouse_pos_filled = false;

    for (const auto& event : window.get_input_events()) {
        switch (event.from) {
            case mgm::MgmWindow::InputEvent::From::MOUSE: {
                if (event.input == mgm::MgmWindow::InputInterface::Mouse_SCROLL_UP || event.input == mgm::MgmWindow::InputInterface::Mouse_SCROLL_DOWN) {
                    io.MouseWheel += event.input == mgm::MgmWindow::InputInterface::Mouse_SCROLL_UP ? 1.0f : -1.0f;
                    break;
                }
                switch (event.mode) {
                    case mgm::MgmWindow::InputEvent::Mode::PRESS:
                        io.MouseDown[input_interface_to_imgui_mb(event.input)] = true;
                        break;
                    case mgm::MgmWindow::InputEvent::Mode::RELEASE:
                        io.MouseDown[input_interface_to_imgui_mb(event.input)] = false;
                        break;
                    case mgm::MgmWindow::InputEvent::Mode::OTHER:
                        if (mouse_pos_filled) {
                            io.MousePos = {mouse_pos_x, (event.value + 1.0f) * 0.5f * io.DisplaySize.y};
                            mouse_pos_filled = false;
                        }
                        else {
                            mouse_pos_x = (event.value + 1.0f) * 0.5f * io.DisplaySize.x;
                            mouse_pos_filled = true;
                        }
                        break;
                    default: break;
                }
                break;
            }
            case mgm::MgmWindow::InputEvent::From::KEYBOARD: {
                switch (event.mode) {
                    case mgm::MgmWindow::InputEvent::Mode::PRESS: {
                        const auto key = input_interface_to_imgui_key(event.input);
                        if (key == ImGuiKey_None)
                            break;
                        io.AddKeyEvent(key, true);
                        break;
                    }
                    case mgm::MgmWindow::InputEvent::Mode::RELEASE: {
                        const auto key = input_interface_to_imgui_key(event.input);
                        if (key == ImGuiKey_None)
                            break;
                        io.AddKeyEvent(key, false);
                        break;
                    }
                    default: break;
                }
                break;
            }
            default: break;
        }
    }
    
    const auto& text = window.get_text_input();
    io.AddInputCharactersUTF8(text.c_str());
}
