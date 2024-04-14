#pragma once
#include "imgui.h"
#include "mgmgpu.hpp"
#include "mgmwin.hpp"

#ifndef IMGUI_DISABLE

// Backend API
IMGUI_IMPL_API bool     ImGui_ImplMgmGFX_Init(mgm::MgmGPU& backend);
IMGUI_IMPL_API void     ImGui_ImplMgmGFX_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplMgmGFX_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplMgmGFX_RenderDrawData(ImDrawData* draw_data);
IMGUI_IMPL_API void     ImGui_ImplMgmGFX_ProcessInput(mgm::MgmWindow& window);

#endif
