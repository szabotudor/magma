#pragma once
#include "editor_windows/settings.hpp"
#include "systems.hpp"
#include "engine.hpp"


namespace mgm {
    class EngineManager : public System {
        friend class MagmaEngine;

        Path font_file;
        ImFont* font = nullptr;

        public:
        EngineManager();

        void general_settings() { Logging{"EngineManager"}.log("General settings opened"); }
        SETTINGS_SUBSECTION_DRAW_FUNC(general_settings)

        void render_settings() { Logging{"EngineManager"}.log("Render settings opened"); }
        SETTINGS_SUBSECTION_DRAW_FUNC(render_settings)

#if defined(ENABLE_EDITOR)
        virtual void in_editor_update(float delta) override { update(delta); }
#endif
        void update(float delta) override;


        ~EngineManager() override;
    };
}
