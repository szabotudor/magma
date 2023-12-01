#include "imgui_impl_mgmgfx.h"
#include "imgui.h"
#include "logging.hpp"
#include "mgmath.hpp"
#include "mgmgfx.hpp"
#include "mgmwin.hpp"
#include <cstdint>
#include <stdexcept>



struct ImGui_BackendData {
    mgm::MgmGraphics* backend = nullptr;
    mgm::MgmGraphics::TextureHandle font_atlas{};
    mgm::MgmGraphics::ShaderHandle font_atlas_shader{};
    uint32_t proj_mat_uniform{};
    mgm::Logging log{"ImGui MgmGFX Backend"};
};
ImGui_BackendData* get_backend_data() {
    if (ImGui::GetCurrentContext() != nullptr)
        return (ImGui_BackendData*)ImGui::GetIO().BackendRendererUserData;
    throw std::runtime_error("ImGui backend not initialized");
}

bool ImGui_ImplMgmGFX_Init(mgm::MgmGraphics &backend) {
    auto& io = ImGui::GetIO();
    if (io.BackendRendererUserData != nullptr)
        throw std::runtime_error("ImGui backend already initialized");

    const auto viewport = backend.get_viewport();
    io.DisplaySize = {static_cast<float>(viewport.y().x()), static_cast<float>(viewport.y().y())};

    uint8_t* tex_data = nullptr;
    mgm::vec2i32 size{};
    io.Fonts->GetTexDataAsRGBA32(&tex_data, &size.x(), &size.y());
    const auto fonts_texture = backend.make_texture(
        tex_data,
        mgm::vec2u32{static_cast<unsigned int>(size.x()), static_cast<unsigned int>(size.y())},
        true, false
    );
    io.Fonts->SetTexID((void*)(intptr_t)fonts_texture);

    const auto fonts_shader = backend.make_shader_from_source(
        "#version 460 core\n"
        "layout (location = 0) in vec3 Vert;\n"
        "layout (location = 2) in vec4 VertColor;\n"
        "layout (location = 3) in vec2 TexCoords;\n"
        "uniform mat4 Proj;\n"
        "out vec2 Frag_TexCoords;\n"
        "out vec4 Frag_VertColor;\n"
        "void main() {\n"
        "   Frag_TexCoords = TexCoords;\n"
        "   Frag_VertColor = VertColor;\n"
        "   gl_Position = Proj * vec4(Vert, 1.0f);\n"
        "}\n",

        "#version 460 core\n"
        "in vec2 Frag_TexCoords;\n"
        "in vec4 Frag_VertColor;\n"
        "uniform sampler2D Texture;\n"
        "out vec4 FragColor;"
        "void main() {\n"
        "   FragColor = Frag_VertColor * texture(Texture, Frag_TexCoords);\n"
        "}\n"
    );

    backend.uniform_data(fonts_shader, "Texture", (int32_t)0);

    io.BackendRendererUserData = new ImGui_BackendData{
        &backend,
        fonts_texture,
        fonts_shader,
        backend.find_uniform(fonts_shader, "Proj")
    };
    get_backend_data()->log.log("Initialized ImGui backend");

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
    auto& backend = *get_backend_data()->backend;
}

void ImGui_ImplMgmGFX_RenderDrawData(ImDrawData *draw_data) {
    auto* data = get_backend_data();
    auto& backend = *data->backend;
    const auto viewport = backend.get_viewport();
    ImGui::GetIO().DisplaySize = {static_cast<float>(viewport.y().x()), static_cast<float>(viewport.y().y())};
    mgm::vec2i32 viewport_size {
        static_cast<int>(draw_data->DisplaySize.x * draw_data->FramebufferScale.x),
        static_cast<int>(draw_data->DisplaySize.y * draw_data->FramebufferScale.y)
    };

    const ImVec2 clip_off = draw_data->DisplayPos;
    const ImVec2 clip_scale = draw_data->FramebufferScale;

    const bool blending = backend.is_blending_enabled();
    if (!blending)
        backend.enable_blending();

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
    backend.uniform_data(data->font_atlas_shader, data->proj_mat_uniform, proj);

    for (uint32_t i = 0; i < draw_data->CmdListsCount; i++) {
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

        const auto mesh = backend.make_mesh(
            verts.data(), nullptr,
            colors.data(), coords.data(),
            verts.size(),
            indices.data(), indices.size()
        );

        for (uint32_t j = 0; j < cmd_list->CmdBuffer.Size; j++) {
            const auto* cmd = &cmd_list->CmdBuffer[j];

            if (cmd->UserCallback) {
                cmd->UserCallback(cmd_list, cmd);
            }
            else {
                ImVec2 clip_min((cmd->ClipRect.x - clip_off.x) * clip_scale.x, (cmd->ClipRect.y - clip_off.y) * clip_scale.y);
                ImVec2 clip_max((cmd->ClipRect.z - clip_off.x) * clip_scale.x, (cmd->ClipRect.w - clip_off.y) * clip_scale.y);
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                    continue;

                backend.scissor(
                    {(int)clip_min.x, (int)(viewport_size.y() - clip_max.y)},
                    {(int)(clip_max.x - clip_min.x), (int)(clip_max.y - clip_min.y)}
                );

                backend.change_mesh_indices(mesh, indices.data() + cmd->IdxOffset, cmd->ElemCount);
                backend.draw(mesh, data->font_atlas_shader, {data->font_atlas});
            }
        }
        backend.destroy_mesh(mesh);
    }

    backend.viewport(viewport[0], viewport[1]);
    backend.scissor(viewport[0], viewport[1]);

    if (!blending)
        backend.disable_blending();
}


ImGuiKey input_interface_to_imgui_key(const mgm::MgmWindow::InputInterface keycode) {
    switch (keycode) {
        case mgm::MgmWindow::InputInterface::Key_TAB: return ImGuiKey_Tab;
        case mgm::MgmWindow::InputInterface::Key_ARROW_LEFT: return ImGuiKey_LeftArrow;
        case mgm::MgmWindow::InputInterface::Key_ARROW_RIGHT: return ImGuiKey_RightArrow;
        case mgm::MgmWindow::InputInterface::Key_ARROW_UP: return ImGuiKey_UpArrow;
        case mgm::MgmWindow::InputInterface::Key_ARROW_DOWN: return ImGuiKey_DownArrow;
        case mgm::MgmWindow::InputInterface::Key_PAGEUP: return ImGuiKey_PageUp;
        case mgm::MgmWindow::InputInterface::Key_PAGEDOWN: return ImGuiKey_PageDown;
        case mgm::MgmWindow::InputInterface::Key_HOME: return ImGuiKey_Home;
        case mgm::MgmWindow::InputInterface::Key_END: return ImGuiKey_End;
        case mgm::MgmWindow::InputInterface::Key_INSERT: return ImGuiKey_Insert;
        case mgm::MgmWindow::InputInterface::Key_DELETE: return ImGuiKey_Delete;
        case mgm::MgmWindow::InputInterface::Key_BACKSPACE: return ImGuiKey_Backspace;
        case mgm::MgmWindow::InputInterface::Key_SPACE: return ImGuiKey_Space;
        case mgm::MgmWindow::InputInterface::Key_ENTER: return ImGuiKey_Enter;
        case mgm::MgmWindow::InputInterface::Key_ESC: return ImGuiKey_Escape;
        case mgm::MgmWindow::InputInterface::Key_QUOTE: return ImGuiKey_Apostrophe;
        case mgm::MgmWindow::InputInterface::Key_COMMA: return ImGuiKey_Comma;
        case mgm::MgmWindow::InputInterface::Key_PERIOD: return ImGuiKey_Period;
        case mgm::MgmWindow::InputInterface::Key_FORWARD_SLASH: return ImGuiKey_Slash;
        case mgm::MgmWindow::InputInterface::Key_SEMICOLON: return ImGuiKey_Semicolon;
        case mgm::MgmWindow::InputInterface::Key_EQUAL: return ImGuiKey_Equal;
        case mgm::MgmWindow::InputInterface::Key_OPEN_BRACKET: return ImGuiKey_LeftBracket;
        case mgm::MgmWindow::InputInterface::Key_BACKSLASH: return ImGuiKey_Backslash;
        case mgm::MgmWindow::InputInterface::Key_CLOSE_BRACKET: return ImGuiKey_RightBracket;
        case mgm::MgmWindow::InputInterface::Key_GRAVE: return ImGuiKey_GraveAccent;
        case mgm::MgmWindow::InputInterface::Key_CAPSLOCK: return ImGuiKey_CapsLock;
        case mgm::MgmWindow::InputInterface::Key_SCROLLLOCK: return ImGuiKey_ScrollLock;
        case mgm::MgmWindow::InputInterface::Key_NUMLOCK: return ImGuiKey_NumLock;
        case mgm::MgmWindow::InputInterface::Key_CTRL: return ImGuiKey_LeftCtrl;
        case mgm::MgmWindow::InputInterface::Key_SHIFT: return ImGuiKey_LeftShift;
        case mgm::MgmWindow::InputInterface::Key_ALT: return ImGuiKey_LeftAlt;
        case mgm::MgmWindow::InputInterface::Key_META: return ImGuiKey_LeftSuper;
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
        case mgm::MgmWindow::InputInterface::Key_PLUS: return ImGuiKey_KeypadAdd;
        case mgm::MgmWindow::InputInterface::Key_MINUS: return ImGuiKey_Minus;

        default: return ImGuiKey_None;
    }
}

char input_interface_to_char(const mgm::MgmWindow& window, const mgm::MgmWindow::InputInterface key) {
    if (window.get_input_interface(mgm::MgmWindow::InputInterface::Key_CTRL) > 0.0f
    || window.get_input_interface(mgm::MgmWindow::InputInterface::Key_ALT) > 0.0f)
        return '\0';
    switch (key) {
        case mgm::MgmWindow::InputInterface::Key_0: return '0';
        case mgm::MgmWindow::InputInterface::Key_1: return '1';
        case mgm::MgmWindow::InputInterface::Key_2: return '2';
        case mgm::MgmWindow::InputInterface::Key_3: return '3';
        case mgm::MgmWindow::InputInterface::Key_4: return '4';
        case mgm::MgmWindow::InputInterface::Key_5: return '5';
        case mgm::MgmWindow::InputInterface::Key_6: return '6';
        case mgm::MgmWindow::InputInterface::Key_7: return '7';
        case mgm::MgmWindow::InputInterface::Key_8: return '8';
        case mgm::MgmWindow::InputInterface::Key_9: return '9';
        case mgm::MgmWindow::InputInterface::Key_A: return window.get_input_interface(mgm::MgmWindow::InputInterface::Key_SHIFT) > 0.0f ? 'A' : 'a';
        case mgm::MgmWindow::InputInterface::Key_B: return window.get_input_interface(mgm::MgmWindow::InputInterface::Key_SHIFT) > 0.0f ? 'B' : 'b';
        case mgm::MgmWindow::InputInterface::Key_C: return window.get_input_interface(mgm::MgmWindow::InputInterface::Key_SHIFT) > 0.0f ? 'C' : 'c';
        case mgm::MgmWindow::InputInterface::Key_D: return window.get_input_interface(mgm::MgmWindow::InputInterface::Key_SHIFT) > 0.0f ? 'D' : 'd';
        case mgm::MgmWindow::InputInterface::Key_E: return window.get_input_interface(mgm::MgmWindow::InputInterface::Key_SHIFT) > 0.0f ? 'E' : 'e';
        case mgm::MgmWindow::InputInterface::Key_F: return window.get_input_interface(mgm::MgmWindow::InputInterface::Key_SHIFT) > 0.0f ? 'F' : 'f';
        case mgm::MgmWindow::InputInterface::Key_G: return window.get_input_interface(mgm::MgmWindow::InputInterface::Key_SHIFT) > 0.0f ? 'G' : 'g';
        case mgm::MgmWindow::InputInterface::Key_H: return window.get_input_interface(mgm::MgmWindow::InputInterface::Key_SHIFT) > 0.0f ? 'H' : 'h';
        case mgm::MgmWindow::InputInterface::Key_I: return window.get_input_interface(mgm::MgmWindow::InputInterface::Key_SHIFT) > 0.0f ? 'I' : 'i';
        case mgm::MgmWindow::InputInterface::Key_J: return window.get_input_interface(mgm::MgmWindow::InputInterface::Key_SHIFT) > 0.0f ? 'J' : 'j';
        case mgm::MgmWindow::InputInterface::Key_K: return window.get_input_interface(mgm::MgmWindow::InputInterface::Key_SHIFT) > 0.0f ? 'K' : 'k';
        case mgm::MgmWindow::InputInterface::Key_L: return window.get_input_interface(mgm::MgmWindow::InputInterface::Key_SHIFT) > 0.0f ? 'L' : 'l';
        case mgm::MgmWindow::InputInterface::Key_M: return window.get_input_interface(mgm::MgmWindow::InputInterface::Key_SHIFT) > 0.0f ? 'M' : 'm';
        case mgm::MgmWindow::InputInterface::Key_N: return window.get_input_interface(mgm::MgmWindow::InputInterface::Key_SHIFT) > 0.0f ? 'N' : 'n';
        case mgm::MgmWindow::InputInterface::Key_O: return window.get_input_interface(mgm::MgmWindow::InputInterface::Key_SHIFT) > 0.0f ? 'O' : 'o';
        case mgm::MgmWindow::InputInterface::Key_P: return window.get_input_interface(mgm::MgmWindow::InputInterface::Key_SHIFT) > 0.0f ? 'P' : 'p';
        case mgm::MgmWindow::InputInterface::Key_Q: return window.get_input_interface(mgm::MgmWindow::InputInterface::Key_SHIFT) > 0.0f ? 'Q' : 'q';
        case mgm::MgmWindow::InputInterface::Key_R: return window.get_input_interface(mgm::MgmWindow::InputInterface::Key_SHIFT) > 0.0f ? 'R' : 'r';
        case mgm::MgmWindow::InputInterface::Key_S: return window.get_input_interface(mgm::MgmWindow::InputInterface::Key_SHIFT) > 0.0f ? 'S' : 's';
        case mgm::MgmWindow::InputInterface::Key_T: return window.get_input_interface(mgm::MgmWindow::InputInterface::Key_SHIFT) > 0.0f ? 'T' : 't';
        case mgm::MgmWindow::InputInterface::Key_U: return window.get_input_interface(mgm::MgmWindow::InputInterface::Key_SHIFT) > 0.0f ? 'U' : 'u';
        case mgm::MgmWindow::InputInterface::Key_V: return window.get_input_interface(mgm::MgmWindow::InputInterface::Key_SHIFT) > 0.0f ? 'V' : 'v';
        case mgm::MgmWindow::InputInterface::Key_W: return window.get_input_interface(mgm::MgmWindow::InputInterface::Key_SHIFT) > 0.0f ? 'W' : 'w';
        case mgm::MgmWindow::InputInterface::Key_X: return window.get_input_interface(mgm::MgmWindow::InputInterface::Key_SHIFT) > 0.0f ? 'X' : 'x';
        case mgm::MgmWindow::InputInterface::Key_Y: return window.get_input_interface(mgm::MgmWindow::InputInterface::Key_SHIFT) > 0.0f ? 'Y' : 'y';
        case mgm::MgmWindow::InputInterface::Key_Z: return window.get_input_interface(mgm::MgmWindow::InputInterface::Key_SHIFT) > 0.0f ? 'Z' : 'z';
        case mgm::MgmWindow::InputInterface::Key_PLUS: return '+';
        case mgm::MgmWindow::InputInterface::Key_MINUS: return '-';
        case mgm::MgmWindow::InputInterface::Key_ASTERISK: return '*';
        case mgm::MgmWindow::InputInterface::Key_EQUAL: return '=';
        case mgm::MgmWindow::InputInterface::Key_COMMA: return ',';
        case mgm::MgmWindow::InputInterface::Key_PERIOD: return '.';
        case mgm::MgmWindow::InputInterface::Key_COLON: return ':';
        case mgm::MgmWindow::InputInterface::Key_SEMICOLON: return ';';
        case mgm::MgmWindow::InputInterface::Key_APOSTROPHE: return '\'';
        case mgm::MgmWindow::InputInterface::Key_QUOTE: return '"';
        case mgm::MgmWindow::InputInterface::Key_OPEN_CURLY_BRACKET: return '{';
        case mgm::MgmWindow::InputInterface::Key_CLOSE_CURLY_BRACKET: return '}';
        case mgm::MgmWindow::InputInterface::Key_OPEN_BRACKET: return '[';
        case mgm::MgmWindow::InputInterface::Key_CLOSE_BRACKET: return ']';
        case mgm::MgmWindow::InputInterface::Key_BACKSLASH: return '\\';
        case mgm::MgmWindow::InputInterface::Key_FORWARD_SLASH: return '/';
        case mgm::MgmWindow::InputInterface::Key_QUESTION_MARK: return '?';
        case mgm::MgmWindow::InputInterface::Key_EXCLAMATION_MARK: return '!';
        case mgm::MgmWindow::InputInterface::Key_AT: return '@';
        case mgm::MgmWindow::InputInterface::Key_HASH: return '#';
        case mgm::MgmWindow::InputInterface::Key_DOLLAR: return '$';
        case mgm::MgmWindow::InputInterface::Key_PERCENT: return '%';
        case mgm::MgmWindow::InputInterface::Key_CARET: return '^';
        case mgm::MgmWindow::InputInterface::Key_GREATER: return '>';
        case mgm::MgmWindow::InputInterface::Key_LESS: return '<';
        case mgm::MgmWindow::InputInterface::Key_AMPERSAND: return '&';
        case mgm::MgmWindow::InputInterface::Key_OPEN_PARENTHESIS: return '(';
        case mgm::MgmWindow::InputInterface::Key_CLOSE_PARENTHESIS: return ')';
        case mgm::MgmWindow::InputInterface::Key_UNDERSCORE: return '_';
        case mgm::MgmWindow::InputInterface::Key_GRAVE: return '`';
        case mgm::MgmWindow::InputInterface::Key_TILDE: return '~';
        case mgm::MgmWindow::InputInterface::Key_VERTICAL_LINE: return '|';
        default: return '\0';
    }
}

ImGuiMouseButton input_interface_to_imgui_mouse_button(mgm::MgmWindow::InputInterface button) {
    switch (button) {
        case mgm::MgmWindow::InputInterface::Mouse_LEFT: return ImGuiMouseButton_Left;
        case mgm::MgmWindow::InputInterface::Mouse_MIDDLE: return ImGuiMouseButton_Middle;
        case mgm::MgmWindow::InputInterface::Mouse_RIGHT: return ImGuiMouseButton_Right;
        case mgm::MgmWindow::InputInterface::Mouse_SCROLL_UP:
        case mgm::MgmWindow::InputInterface::Mouse_SCROLL_DOWN:
        default: return ImGuiMouseButton_COUNT;
    }
}

void ImGui_ImplMgmGFX_ProcessInput(mgm::MgmWindow &window) {
    auto& io = ImGui::GetIO();
    auto* data = get_backend_data();

    io.MousePos = {
        (window.get_input_interface(mgm::MgmWindow::InputInterface::Mouse_POS_X) + 1.0f) * static_cast<float>(window.get_size().x()) * 0.5f,
        (window.get_input_interface(mgm::MgmWindow::InputInterface::Mouse_POS_Y) + 1.0f) * static_cast<float>(window.get_size().y()) * 0.5f
    };

    for (const auto& event : window.get_input_events()) {
        switch (event.from) {
            case mgm::MgmWindow::InputEvent::From::KEYBOARD: {
                io.AddKeyEvent(input_interface_to_imgui_key(event.interface), event.mode == mgm::MgmWindow::InputEvent::Mode::PRESS);
                if (event.mode == mgm::MgmWindow::InputEvent::Mode::PRESS) {
                    const auto c = input_interface_to_char(window, event.interface);
                    if (c != '\0')
                        io.AddInputCharacter(c);
                }
                break;
            }
            case mgm::MgmWindow::InputEvent::From::MOUSE: {
                const auto button = input_interface_to_imgui_mouse_button(event.interface);
                if (button == ImGuiMouseButton_COUNT)
                    break;
                io.AddMouseButtonEvent(button, event.mode == mgm::MgmWindow::InputEvent::Mode::PRESS);
                break;
            }
            default: {
                break;
            }
        }
    }
}
