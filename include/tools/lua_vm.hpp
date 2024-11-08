#pragma once
#include "lua.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <lauxlib.h>
#include <lua.h>
#include <mutex>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>


class lua_State;


namespace mgm {
    class LuaVM {
        lua_State* state = nullptr;
        mutable std::recursive_mutex mutex{};
        bool registering_type = false;

        public:
        LuaVM();

        LuaVM(const LuaVM&) = delete;
        LuaVM& operator=(const LuaVM&) = delete;

        LuaVM(LuaVM&& other) noexcept {
            std::lock_guard lock{other.mutex};
            std::lock_guard lock2{mutex};

            state = other.state;
            other.state = nullptr;
            registered_types = std::move(other.registered_types);

            lua_pushlightuserdata(state, this);
            lua_setglobal(state, "__LuaVM");
        }
        LuaVM& operator=(LuaVM&& other) noexcept {
            if (this == &other)
                return *this;

            std::lock_guard lock{other.mutex};
            std::lock_guard lock2{mutex};

            state = other.state;
            other.state = nullptr;
            registered_types = std::move(other.registered_types);

            lua_pushlightuserdata(state, this);
            lua_setglobal(state, "__LuaVM");

            return *this;
        }

        void init_state();
        void reset_state();

        void script(const std::string& script);

        private:
        struct RegisteredType {
        private:
            friend class LuaVM;

            std::string name{};
            std::unordered_map<std::string, std::function<void(LuaVM& vm, void* _obj)>> indexers{};
            std::unordered_map<std::string, std::function<void(LuaVM& vm, void* _obj)>> newindexers{};

        public:

        };
        std::unordered_map<size_t, RegisteredType> registered_types{};


        template<typename Return, typename T, typename... Args, std::size_t... Is>
        Return call_member_function(T* obj, Return (T::*field)(Args...), std::index_sequence<Is...>) {
            return (obj->*field)(lua_checkvar<Args>(Is + 2)...);
        }

        template<typename Return, typename... Args, std::size_t... Is>
        Return call_function(Return(*func)(Args...), std::index_sequence<Is...>) {
            return func(lua_checkvar<Args>(Is + 1)...);
        }

        template<typename T, typename C, typename Return, typename... Args, std::enable_if_t<std::is_same_v<T, Return (C::*)(Args...)>, bool> = true>
        void lua_pushvar(Return (C::*field)(Args...)) {
            std::lock_guard lock{mutex};

            using V = Return (C::*)(Args...);

            std::array<uint8_t, sizeof(V)> arr{};
            std::memcpy(arr.data(), &field, sizeof(V));

            lua_pushvar<std::array<uint8_t, sizeof(V)>>(arr);

            lua_pushcclosure(state, [](lua_State* L) {
                const auto argc = lua_gettop(L);
                if (argc != sizeof...(Args) + 1)
                    return luaL_error(L, "Expected %d arguments, got %d", sizeof...(Args), argc - 1);
                
                lua_getglobal(L, "__LuaVM");
                LuaVM* vm = static_cast<LuaVM*>(lua_touserdata(L, -1));
                lua_pop(L, 1);

                const std::array<uint8_t, sizeof(V)>& args = vm->lua_checkvar<std::array<uint8_t, sizeof(V)>>(lua_upvalueindex(1));

                V field_offset{};
                std::memcpy(&field_offset, args.data(), sizeof(V));

                C* obj = nullptr;
                
                const auto it = vm->registered_types.find(typeid(C).hash_code());
                if (it == vm->registered_types.end())
                    obj = static_cast<C*>(lua_touserdata(L, 1));
                else
                    obj = static_cast<C*>(luaL_checkudata(L, 1, it->second.name.c_str()));

                if constexpr (std::is_same_v<Return, void>) {
                    vm->call_member_function(obj, field_offset, std::index_sequence_for<Args...>{});
                    return 0;
                }
                else {
                    vm->lua_pushvar<Return>(vm->call_member_function(obj, field_offset, std::index_sequence_for<Args...>{}));
                    return 1;
                }
            }, 1);
        }

        template<typename T, typename Return, typename... Args, std::enable_if_t<std::is_same_v<Return(*)(Args...), T>, bool> = true>
        void lua_pushvar(Return(*function)(Args...)) {
            std::lock_guard lock{mutex};

            using V = Return(*)(Args...);

            std::array<uint8_t, sizeof(V)> arr{};
            std::memcpy(arr.data(), &function, sizeof(V));

            lua_pushvar<std::array<uint8_t, sizeof(V)>>(arr);

            lua_pushcclosure(state, [](lua_State* L) {
                const auto argc = lua_gettop(L);
                if (argc != sizeof...(Args))
                    return luaL_error(L, "Expected %d arguments, got %d", sizeof...(Args), argc);
                
                lua_getglobal(L, "__LuaVM");
                LuaVM* vm = static_cast<LuaVM*>(lua_touserdata(L, -1));
                lua_pop(L, 1);

                const std::array<uint8_t, sizeof(V)>& args = vm->lua_checkvar<std::array<uint8_t, sizeof(V)>>(lua_upvalueindex(1));

                V func{};
                std::memcpy(&func, args.data(), sizeof(V));

                if constexpr (std::is_same_v<Return, void>) {
                    vm->call_function(func, std::index_sequence_for<Args...>{});
                    return 0;
                }
                else {
                    vm->lua_pushvar<Return>(vm->call_function(func, std::index_sequence_for<Args...>{}));
                    return 1;
                }
            }, 1);
        }

        template<typename T, typename... Ts, std::enable_if_t<std::is_constructible_v<T, Ts...>
        && !std::is_function_v<T> && !std::is_member_function_pointer_v<T> && !std::is_member_object_pointer_v<T> && !std::is_pointer_v<T> && !std::is_reference_v<T>,
        bool> = true>
        void lua_pushvar(Ts&&... args) {
            std::lock_guard lock{mutex};

            if constexpr (std::is_same_v<T, uint8_t> || std::is_same_v<T, int8_t>
            || std::is_same_v<T, uint16_t> || std::is_same_v<T, int16_t>
            || std::is_same_v<T, uint32_t> || std::is_same_v<T, int32_t>
            || std::is_same_v<T, uint64_t> || std::is_same_v<T, int64_t>)
                lua_pushinteger(state, lua_Integer(args...));

            else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double> || std::is_same_v<T, long double>)
                lua_pushnumber(state, lua_Number(args...));

            else if constexpr (std::is_same_v<T, bool>)
                lua_pushboolean(state, bool(args...));

            else if constexpr (std::is_same_v<T, const char*>)
                lua_pushstring(state, T(args...));
            else if constexpr (std::is_same_v<T, std::string>)
                lua_pushstring(state, std::string(args...).c_str());

            else {
                T* obj = static_cast<T*>(lua_newuserdata(state, sizeof(T)));
                new (obj) T(args...);
                if (registered_types.contains(typeid(T).hash_code()))
                    luaL_setmetatable(state, get_type_name<T>().c_str());
                else {
                    lua_newtable(state);
                    lua_pushcfunction(state, [](lua_State* L) {
                        T* ptr = static_cast<T*>(lua_touserdata(L, 1));
                        ptr->~T();
                        return 0;
                    });
                    lua_setfield(state, -2, "__gc");
                    lua_setmetatable(state, -2);
                }
            }
        }

        template<typename T>
        auto lua_checkvar(int idx) const -> std::conditional_t<std::is_class_v<T> && !std::is_same_v<T, std::string>, T&, T> {
            std::lock_guard lock{mutex};

            if constexpr (std::is_same_v<T, uint8_t> || std::is_same_v<T, int8_t>
            || std::is_same_v<T, uint16_t> || std::is_same_v<T, int16_t>
            || std::is_same_v<T, uint32_t> || std::is_same_v<T, int32_t>
            || std::is_same_v<T, uint64_t> || std::is_same_v<T, int64_t>)
                return static_cast<T>(luaL_checkinteger(state, idx));

            else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double> || std::is_same_v<T, long double>)
                return static_cast<T>(luaL_checknumber(state, idx));

            else if constexpr (std::is_same_v<T, bool>)
                return lua_toboolean(state, idx) != 0;

            else if constexpr (std::is_same_v<T, const char*>)
                return luaL_checkstring(state, idx);
            else if constexpr (std::is_same_v<T, std::string>)
                return std::string(luaL_checkstring(state, idx));

            else
            {
                const auto it = registered_types.find(typeid(T).hash_code());
                if (it == registered_types.end())
                    return *static_cast<T*>(lua_touserdata(state, idx));
                else
                    return *static_cast<T*>(luaL_checkudata(state, idx, it->second.name.c_str()));
            }
        }

        template<typename T>
        std::string get_type_name() const {
            std::lock_guard lock{mutex};
            return registered_types.at(typeid(T).hash_code()).name;
        }

        template<typename T, typename U, typename... Ts>
        void add_members_to_metatable(const std::string& field_name, U&& field, Ts&&... fields) {
            register_member_for_type<T>(field_name, field);
            add_members_to_metatable<T>(std::forward<Ts>(fields)...);
        }
        template<typename T, typename U>
        void add_members_to_metatable(const std::string& field_name, U&& field) {
            register_member_for_type<T>(field_name, field);
        }

    public:
        template<typename T, typename U>
        void register_member_for_type(const std::string& field_name, U T::* field_offset) {
            std::lock_guard lock{mutex};
            
            auto& type = registered_types[typeid(T).hash_code()];

            if (!registering_type)
                lua_getglobal(state, type.name.c_str());
            
            type.indexers.emplace(field_name, [field_name, field_offset](LuaVM& vm, void* _obj) {
                T* obj = static_cast<T*>(_obj);
                vm.lua_pushvar<U>(obj->*field_offset);
            });
            type.newindexers.emplace(field_name, [field_name, field_offset](LuaVM& vm, void* _obj) {
                T* obj = static_cast<T*>(_obj);
                obj->*field_offset = vm.lua_checkvar<int>(3);
            });

            if (!registering_type)
                lua_pop(state, 1);
        }

        template<typename T, typename Return, typename... Args>
        void register_member_for_type(const std::string& field_name, Return (T::*field)(Args...)) {
            std::lock_guard lock{mutex};

            if (!registering_type)
                lua_getglobal(state, get_type_name<T>().c_str());

            lua_pushvar(field);
            lua_setfield(state, -2, field_name.c_str());

            if (!registering_type)
                lua_pop(state, 1);
        }

        template<typename T, typename... Ts>
        int register_type(const std::string& type_name, Ts&&... fields) {
            std::lock_guard lock{mutex};
            registering_type = true;

            registered_types[typeid(T).hash_code()].name = type_name;

            if (!luaL_newmetatable(state, type_name.c_str())) {
                registering_type = false;
                return luaL_error(state, "Failed to create metatable for type %s", type_name.c_str());
            }
            
            lua_pushstring(state, type_name.c_str());
            lua_setfield(state, -2, "__name");

            lua_pushcfunction(state, [](lua_State* L) {
                T* ptr = static_cast<T*>(lua_touserdata(L, 1));
                const char* field = luaL_checkstring(L, 2);

                lua_getglobal(L, "__LuaVM");
                LuaVM* vm = static_cast<LuaVM*>(lua_touserdata(L, -1));
                lua_pop(L, 1);

                auto& type = vm->registered_types[typeid(T).hash_code()];

                const auto it = type.indexers.find(field);
                if (it == type.indexers.end()) {
                    lua_getmetatable(L, 1);
                    lua_pushstring(L, field);
                    lua_rawget(L, -2);

                    if (lua_isnil(L, -1))
                        return luaL_error(L, "Field %s does not exist", field);

                    lua_remove(L, -2);
                    return 1;
                }

                if (it == type.indexers.end())
                    return luaL_error(L, "Field %s does not exist", field);

                it->second(*vm, ptr);
                return 1;
            });
            lua_setfield(state, -2, "__index");

            lua_pushcfunction(state, [](lua_State* L) {
                T* ptr = static_cast<T*>(lua_touserdata(L, 1));
                const char* field = luaL_checkstring(L, 2);

                lua_getglobal(L, "__LuaVM");
                LuaVM* vm = static_cast<LuaVM*>(lua_touserdata(L, -1));
                lua_pop(L, 1);

                auto& type = vm->registered_types[typeid(T).hash_code()];
                const auto it = type.newindexers.find(field);

                if (it == type.newindexers.end())
                    return luaL_error(L, "Field %s does not exist", field);

                it->second(*vm, ptr);
                return 0;
            });
            lua_setfield(state, -2, "__newindex");

            if constexpr (sizeof...(Ts) > 0)
                add_members_to_metatable<T>(std::forward<Ts>(fields)...);

            lua_pushcfunction(state, [](lua_State* L) {
                T* ptr = static_cast<T*>(lua_touserdata(L, 1));
                ptr->~T();
                return 0;
            });
            lua_setfield(state, -2, "__gc");

            if constexpr (std::is_default_constructible_v<T>) {
                lua_pushcfunction(state, [](lua_State* L) {
                    lua_getglobal(L, "__LuaVM");
                    LuaVM* vm = static_cast<LuaVM*>(lua_touserdata(L, -1));
                    lua_pop(L, 1);
                    T* ptr = static_cast<T*>(lua_newuserdata(L, sizeof(T)));
                    new (ptr) T{};
                    luaL_setmetatable(L, vm->get_type_name<T>().c_str());
                    return 1;
                });
                lua_setfield(state, -2, "new");
            }

            lua_setglobal(state, type_name.c_str());

            registering_type = false;
            return 0;
        }

        template<typename T>
        auto get_var(const std::string& variable_path) const {
            std::lock_guard lock{mutex};

            std::vector<std::string> fields{};
            size_t pos = 0;

            while (pos < variable_path.size()) {
                const auto end = variable_path.find('.', pos);
                if (end == std::string::npos) {
                    fields.emplace_back(variable_path.substr(pos));
                    break;
                }
                fields.emplace_back(variable_path.substr(pos, end - pos));
                pos = end + 1;
            }

            if (fields.size() == 1) {
                lua_getglobal(state, variable_path.c_str());
                auto res = lua_checkvar<T>(-1);
                lua_pop(state, 1);
                return res;
            }

            lua_getglobal(state, fields.front().c_str());
            int count = 1;

            for (size_t i = 1; i < fields.size(); i++) {
                lua_getfield(state, -1, fields[i].c_str());
                ++count;
                if (!lua_istable(state, -1) && i != fields.size() - 1) {
                    lua_pop(state, count);
                    return T{};
                }
            }
            
            auto res = lua_checkvar<T>(-1);
            lua_pop(state, count);
            return res;
        }

        template<typename T, typename... Ts, std::enable_if_t<std::is_constructible_v<T, Ts...>, bool> = true>
        void set_var(const std::string& variable_path, Ts&&... args) {
            std::lock_guard lock{mutex};

            std::vector<std::string> fields{};
            size_t pos = 0;

            while (pos < variable_path.size()) {
                const auto end = variable_path.find('.', pos);
                if (end == std::string::npos) {
                    fields.emplace_back(variable_path.substr(pos));
                    break;
                }
                fields.emplace_back(variable_path.substr(pos, end - pos));
                pos = end + 1;
            }

            if (fields.size() == 1) {
                lua_pushvar<T>(std::forward<Ts>(args)...);
                lua_setglobal(state, variable_path.c_str());
                return;
            }

            lua_getglobal(state, fields.front().c_str());
            int count = 1;

            for (size_t i = 1; i < fields.size() - 1; i++) {
                lua_getfield(state, -1, fields[i].c_str());
                ++count;
                if (!lua_istable(state, -1) && i != fields.size() - 1) {
                    lua_newtable(state);
                    lua_setfield(state, -2, fields[i].c_str());
                }
            }

            lua_pushvar<T>(std::forward<Ts>(args)...);
            lua_setfield(state, -2, fields.back().c_str());
            lua_pop(state, count);
        }

        bool var_exists(const std::string& variable_path) const {
            std::lock_guard lock{mutex};

            std::vector<std::string> fields{};
            size_t pos = 0;

            while (pos < variable_path.size()) {
                const auto end = variable_path.find('.', pos);
                if (end == std::string::npos) {
                    fields.emplace_back(variable_path.substr(pos));
                    break;
                }
                fields.emplace_back(variable_path.substr(pos, end - pos));
                pos = end + 1;
            }

            if (fields.size() == 1) {
                lua_getglobal(state, variable_path.c_str());
                const bool res = !lua_isnil(state, -1);
                lua_pop(state, 1);
                return res;
            }

            lua_getglobal(state, fields.front().c_str());
            int count = 1;

            for (size_t i = 1; i < fields.size(); i++) {
                lua_getfield(state, -1, fields[i].c_str());
                ++count;
                if (!lua_istable(state, -1) && i != fields.size() - 1) {
                    lua_pop(state, count);
                    return false;
                }
            }

            lua_pop(state, count);
            return true;
        }

        template<typename VM_t>
        struct LuaRef {
        private:
            friend class LuaVM;

            VM_t& vm;
            std::string var_path{};

            LuaRef(VM_t& lua_vm, const std::string& variable_path) : vm{lua_vm}, var_path{variable_path} {}

        public:
            LuaRef<VM_t> operator[](const std::string& field) const {
                return LuaRef<VM_t>{vm, var_path + "." + field};
            }
            LuaRef<VM_t> operator[](const char* field) const {
                return LuaRef<VM_t>{vm, var_path + "." + field};
            }

            std::string path() const {
                return var_path;
            }

            template<typename T, typename VM = VM_t, std::enable_if_t<!std::is_const_v<VM>, bool> = true>
            LuaRef<VM_t>& operator=(T&& value) {
                vm.template set_var<T>(var_path, std::forward<T>(value));
                return *this;
            }

            template<typename T>
            operator T() const {
                if constexpr (std::is_default_constructible_v<T>) {
                    if (!vm.var_exists(var_path))
                        vm.template set_var<T>(var_path);
                }
                else {
                    if (!vm.var_exists(var_path))
                        luaL_error(vm.state, "Variable %s does not exist", var_path.c_str());
                }
                return vm.template get_var<T>(var_path);
            }
        };

        LuaRef<LuaVM> operator[](const std::string& variable_path) {
            return LuaRef{*this, variable_path};
        }
        LuaRef<const LuaVM> operator[](const std::string& variable_path) const {
            return LuaRef{*this, variable_path};
        }

        ~LuaVM();
    };
}
