#pragma once
#include "systems.hpp"
#include "engine.hpp"


namespace mgm {
    class EngineManager : public System {
        friend class MagmaEngine;

        Path font_file;
        ImFont* font = nullptr;

        public:
        EngineManager();

#if defined(ENABLE_EDITOR)
        virtual void in_editor_update(float delta) override { update(delta); }
        void draw_settings_window_contents() override;
#endif
        void update(float delta) override;


        ~EngineManager() override;
    };
}
