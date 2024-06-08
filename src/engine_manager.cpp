#include "engine_manager.hpp"
#include "engine.hpp"
#include "imgui.h"
#include "json.hpp"


namespace mgm {
    EngineManager::EngineManager() {
        system_name = "Engine Manager";
        MagmaEngine engine{};

        if (!engine.file_io().exists(Path::exe_dir / "editor_config.json"))
            return;

        JObject obj = engine.file_io().read_text(Path::exe_dir / "editor_config.json");

        if (obj.has("font_file")) {
            font_file = Path{obj["font_file"]};
            if (!font_file.empty())
                font = ImGui::GetIO().Fonts->AddFontFromFileTTF(font_file.platform_path().c_str(), 16.0f);
        }
    }

    void EngineManager::update(float) {
    }

    EngineManager::~EngineManager() {
        MagmaEngine engine{};
        JObject obj{};

        obj["font_file"] = font_file.platform_path();
        const std::string out = obj;

        engine.file_io().write_text(Path::exe_dir / "editor_config.json", out);
    }
}
