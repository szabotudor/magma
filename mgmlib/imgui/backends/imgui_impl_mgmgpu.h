#pragma once
#include "imgui.h"
#include "mgmgpu.hpp"
#include "mgmwin.hpp"

#ifndef IMGUI_DISABLE

struct ExtractedDrawData {
    mgm::mat4f proj{};

    struct Cmd {
        std::vector<mgm::vec3f> verts{};
        std::vector<mgm::vec2f> coords{};
        std::vector<mgm::vec4f> colors{};
        std::vector<uint32_t> indices{};
        
        struct CmdData {
            uint32_t idx_offset{}, elem_count{};
            mgm::GPUSettings::Scissor scissor{};
        };
        std::vector<CmdData> cmd_data{};
    };
    std::vector<Cmd> cmds{};

    bool is_set = false;

    void clear() {
        proj = mgm::mat4f{};
        cmds.clear();
        is_set = false;
    }
};

void extract_draw_data(ImDrawData* draw_data, ExtractedDrawData& out, const mgm::GPUSettings::Viewport& viewport);

// Backend API
IMGUI_IMPL_API bool     ImGui_ImplMgmGFX_Init(mgm::MgmGPU& backend, const std::string& shader_source);
IMGUI_IMPL_API void     ImGui_ImplMgmGFX_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplMgmGFX_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplMgmGFX_RenderDrawData(ExtractedDrawData& draw_data);
IMGUI_IMPL_API void     ImGui_ImplMgmGFX_ProcessInput(mgm::MgmWindow& window);

namespace ImGui {
    void BeginResizeable(const char* name, bool* p_open = nullptr, ImGuiWindowFlags flags = 0);
}

#endif
