#pragma once
#include "engine.hpp"


namespace mgm {
    class GameManager : public System {
        public:
        GameManager() = default;

        void init() override;
        void close() override;

        ~GameManager() override = default;
    };
}
