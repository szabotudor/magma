#include "tools/lua_vm.hpp"
#include "logging.hpp"


namespace mgm {
    LuaVM::LuaVM() {
        init_state();
    }

    void LuaVM::init_state() {
        std::lock_guard lock{mutex};

        if (state != nullptr)
            reset_state();

        state = luaL_newstate();

        luaL_openlibs(state);

        lua_pushlightuserdata(state, this);
        lua_setglobal(state, "__LuaVM");
    }

    void LuaVM::reset_state() {
        std::lock_guard lock{mutex};

        if (state == nullptr)
            return;

        lua_close(state);
        state = nullptr;
    }

    void LuaVM::script(const std::string& script) {
        if (luaL_dostring(state, script.c_str())) {
            const auto error = lua_tostring(state, -1);
            lua_pop(state, 1);
            Logging{"LuaVM"}.error("Error in script: ", error);
        }
    }

    LuaVM::~LuaVM() {
        reset_state();
    }
} // namespace mgm
