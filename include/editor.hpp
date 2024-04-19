#pragma once
#include "engine.hpp"


namespace mgm {
    class Editor : public System {
        public:
        Editor() = default;

        void init() override;
        void in_editor_update(float delta) override;
        void close() override;

        ~Editor() override = default;
    };
}
