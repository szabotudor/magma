#pragma once
#include "imgui.h"
#include "mgmwin.hpp"

#ifndef IMGUI_DISABLE
namespace mgm {
    class MgmGraphics;
}

// Backend API
IMGUI_IMPL_API bool     ImGui_ImplMgmGFX_Init(mgm::MgmGraphics& backend);
IMGUI_IMPL_API void     ImGui_ImplMgmGFX_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplMgmGFX_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplMgmGFX_RenderDrawData(ImDrawData* draw_data);
IMGUI_IMPL_API void     ImGui_ImplMgmGFX_ProcessInput(mgm::MgmWindow& window);

#endif
