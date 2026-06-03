#include "utilities/lua_wrapper.hpp"
#include "amoeba.hpp"
#include "utilities/debug_console.hpp"
#include "utilities/strings.hpp"
#include "utilities/memory.hpp"
#include "utilities/portal.hpp"

#include <cstdint>
#include <cstring>
#include <filesystem>

// Lua wrapper
//
// polymeric 2026

MAKE_SDPORTAL(DATAOFF_glaiel__MewDirector__p_singleton,
    MewDirector *, get_p_mewdirector_singleton
)

// TODO io.write
static int print_lua(lua_State *state) {
    std::string line;
    int n_args = lua_gettop(state);
    for(int i = 1; i <= n_args; i++) {
        size_t str_len;
        const char *str = luaL_tolstring(state, i, &str_len);
        line.append(str, str_len);
        if(i < n_args) {
            line += "\t";
        }
        lua_pop(state, 1);
    }
    lua_pushlightuserdata(state, &LuaWrapper::LuaWrapperKey);
    lua_gettable(state, LUA_REGISTRYINDEX);
    LuaWrapper *wrap = static_cast<LuaWrapper *>(lua_touserdata(state, -1));
    lua_pop(state, 1);

    // TODO split along internal newlines
    wrap->print_stdout_callback(line);

    if(wrap->log_stdio) {
        D::info("{}", line);
    }

    return 0;
}

template<typename T>
static int safe_read_lua(lua_State *state) {
    void *addr = reinterpret_cast<void *>(luaL_checkinteger(state, 1));
    T read_value;
    if(!jf_read<T>(addr, &read_value)) {
        luaL_error(state, "%s", std::format("cannot dereference: {:p}", addr).c_str());
    }
    if constexpr(std::is_integral_v<T>) {
        lua_pushinteger(state, read_value);
    } else {
        lua_pushnumber(state, read_value);
    }
    return 1;
}

template<typename T>
static int unsafe_read_lua(lua_State *state) {
    T *addr = reinterpret_cast<T *>(luaL_checkinteger(state, 1));
    if constexpr(std::is_integral_v<T>) {
        lua_pushinteger(state, *addr);
    } else {
        lua_pushnumber(state, *addr);
    }
    return 1;
}

static const luaL_Reg call_lib_lua[] = {
    {
        "unsafe_invoke_rax_rcx_rdx",
        [](lua_State *state) -> int {
            auto fn = reinterpret_cast<void *(__cdecl *)(void *, void *)>(luaL_checkinteger(state, 1));
            void *rcx = reinterpret_cast<void *>(luaL_checkinteger(state, 2));
            void *rdx = reinterpret_cast<void *>(luaL_checkinteger(state, 3));
            lua_pushinteger(state, reinterpret_cast<lua_Integer>(fn(rcx, rdx)));
            return 1;
        }
    },
    { nullptr, nullptr }
};

static const luaL_Reg mem_lib_lua[] = {
    { "read_u8", safe_read_lua<uint8_t> },
    { "read_u16", safe_read_lua<uint16_t> },
    { "read_u32", safe_read_lua<uint32_t> },
    { "read_u64", safe_read_lua<uint64_t> },
    { "read_i8", safe_read_lua<int8_t> },
    { "read_i16", safe_read_lua<int16_t> },
    { "read_i32", safe_read_lua<int32_t> },
    { "read_i64", safe_read_lua<int64_t> },
    { "read_f32", safe_read_lua<float> },
    { "read_f64", safe_read_lua<double> },
    { "unsafe_read_u8", unsafe_read_lua<uint8_t> },
    { "unsafe_read_u16", unsafe_read_lua<uint16_t> },
    { "unsafe_read_u32", unsafe_read_lua<uint32_t> },
    { "unsafe_read_u64", unsafe_read_lua<uint64_t> },
    { "unsafe_read_i8", unsafe_read_lua<int8_t> },
    { "unsafe_read_i16", unsafe_read_lua<int16_t> },
    { "unsafe_read_i32", unsafe_read_lua<int32_t> },
    { "unsafe_read_i64", unsafe_read_lua<int64_t> },
    { "unsafe_read_f32", unsafe_read_lua<float> },
    { "unsafe_read_f64", unsafe_read_lua<double> },
    {
        "alloc",
        [](lua_State *state) -> int {
            size_t size = luaL_checkinteger(state, 1);
            lua_pushinteger(state, reinterpret_cast<lua_Integer>(host_alloc(size)));
            return 1;
        }
    },
    {
        "unsafe_free",
        [](lua_State *state) -> int {
            void *addr = reinterpret_cast<void *>(luaL_checkinteger(state, 1));
            host_free(addr);
            return 0;
        }
    },
    {
        "unsafe_realloc",
        [](lua_State *state) -> int {
            void *addr = reinterpret_cast<void *>(luaL_checkinteger(state, 1));
            size_t size = luaL_checkinteger(state, 2);
            lua_pushinteger(state, reinterpret_cast<lua_Integer>(host_realloc(addr, size)));
            return 1;
        }
    },
    {
        "unsafe_memset",
        [](lua_State *state) -> int {
            void *addr = reinterpret_cast<void *>(luaL_checkinteger(state, 1));
            char value = static_cast<char>(luaL_checkinteger(state, 2));
            size_t size = luaL_checkinteger(state, 3);
            std::memset(addr, value, size);
            return 0;
        }
    },
    { nullptr, nullptr }
};

static const luaL_Reg mew_lib_lua[] = {
    {
        "get_mewdirector",
        [](lua_State *state) -> int {
            lua_pushinteger(state, reinterpret_cast<lua_Integer>(get_p_mewdirector_singleton()));
            return 1;
        }
    },
    { nullptr, nullptr }
};

LuaWrapper::LuaWrapper() {
    this->init();
}

LuaWrapper::~LuaWrapper() {
    lua_close(this->state);
}

void LuaWrapper::set_print_stdout_callback(PrintFn cb) {
    this->print_stdout_callback = cb;
}

void LuaWrapper::reset() {
    lua_close(this->state);
    this->init();
}

void LuaWrapper::init() {
    this->state = luaL_newstate();
    luaL_openlibs(this->state);

    // Register registry[&LuaWrapperKey] = &this
    lua_pushlightuserdata(this->state, &LuaWrapper::LuaWrapperKey);
    lua_pushlightuserdata(this->state, this);
    lua_settable(this->state, LUA_REGISTRYINDEX);

    // Register our print implementations
    lua_register(this->state, "print", &print_lua);

    // Register our C bindings library
    luaL_requiref(this->state, "amoeba.c", [](lua_State *state) -> int {
        lua_newtable(state);

        luaL_requiref(state, "amoeba.c.mem", [](lua_State *state) -> int {
            luaL_newlib(state, mem_lib_lua);
            return 1;
        }, 0);
        lua_setfield(state, -2, "mem");

        luaL_requiref(state, "amoeba.c.mew", [](lua_State *state) -> int {
            luaL_newlib(state, mew_lib_lua);
            return 1;
        }, 0);
        lua_setfield(state, -2, "mew");

        luaL_requiref(state, "amoeba.c.call", [](lua_State *state) -> int {
            luaL_newlib(state, call_lib_lua);
            return 1;
        }, 0);
        lua_setfield(state, -2, "call");

        return 1;
    }, 0);
    lua_pop(this->state, 1);

    // https://stackoverflow.com/questions/4125971/setting-the-global-lua-path-variable-from-c-c
    lua_getglobal(this->state, "package"); // push
    // Set the module search path to be relative to our dll, not the host exe (LUA_PATH_DEFAULT)
    std::string path;
    // FIXME not correct during static init
    std::filesystem::path dll_parent_dir = get_module_file_path(reinterpret_cast<HMODULE>(G.dll_base_va)).parent_path();
    path.append(convert_filesystem_path_to_utf8_string(dll_parent_dir / "lua" / "?.lua"));
    path.append(";");
    path.append(convert_filesystem_path_to_utf8_string(dll_parent_dir / "lua" / "?" / "init.lua"));
    lua_pushstring(this->state, path.c_str()); // push
    lua_setfield(this->state, -2, "path"); // pop
    // Disable automatic loading of C modules (LUA_CPATH_DEFAULT)
    lua_pushstring(this->state, ""); // push
    lua_setfield(this->state, -2, "cpath"); // pop
    lua_pop(this->state, 1); // pop
}
