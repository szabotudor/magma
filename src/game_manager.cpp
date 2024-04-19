#include "game_manager.hpp"


namespace mgm {
    void GameManager::init() {
        Logging{"GameManager"}.log("Game Manager initialized");
    }

    void GameManager::close() {
        Logging{"GameManager"}.log("Game Manager closed");
    }
}
