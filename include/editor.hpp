#pragma once
#include "engine.hpp"


namespace mgm {
    class Editor : public System {
        public:
        Editor() = default;

        void on_begin_play() override;
        void in_editor_update(float delta) override;
        void on_end_play() override;

        ~Editor() override = default;
    };
}
