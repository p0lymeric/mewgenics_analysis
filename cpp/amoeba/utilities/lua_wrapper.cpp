#include "utilities/lua_wrapper.hpp"
#include "amoeba.hpp"
#include "utilities/debug_console.hpp"
#include "utilities/strings.hpp"
#include "utilities/memory.hpp"
#include "utilities/portal.hpp"

#include <cstdint>
#include <cstring>
#include <filesystem>

#include <windows.h>

#include "ffi.h"

// Lua wrapper
//
// polymeric 2026

MAKE_SDPORTAL(DATAOFF_glaiel__MewDirector__p_singleton,
    MewDirector *, get_p_mewdirector_singleton
)

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

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

// libffi

static const luaL_Reg ffi_CType_lib_lua[] = {
    {
        "make",
        [](lua_State *state) -> int {
            const int ARG_TYPES_IDX = 1;
            luaL_checktype(state, ARG_TYPES_IDX, LUA_TTABLE);
            unsigned int nelements = static_cast<unsigned int>(lua_rawlen(state, ARG_TYPES_IDX));

            ffi_type *p_type = reinterpret_cast<ffi_type *>(lua_newuserdatauv(state, sizeof(ffi_type), 2));

            ffi_type **pp_elements = reinterpret_cast<ffi_type **>(lua_newuserdatauv(state, sizeof(ffi_type *) * (nelements + 1), 0));
            for(unsigned int i = 0; i < nelements; i++) {
                lua_rawgeti(state, ARG_TYPES_IDX, i + 1);
                ffi_type *p_element = reinterpret_cast<ffi_type *>(luaL_testudata(state, -1, "amoeba.c.ffi.CType"));
                if(p_element == nullptr) {
                    luaL_error(state, "elements index %d is not a CType", i + 1);
                }
                pp_elements[i] = p_element;
                lua_pop(state, 1);
            }
            pp_elements[nelements] = nullptr;
            lua_setiuservalue(state, -2, 1);

            // size, alignment are automatically set
            // type must be STRUCT
            p_type->size = 0;
            p_type->alignment = 0;
            p_type->type = FFI_TYPE_STRUCT;
            p_type->elements = pp_elements;

            // Populate a reference tracking array to tie referenced CType lifetime with this CType
            lua_newtable(state);
            for(unsigned int i = 0; i < nelements; i++) {
                lua_rawgeti(state, ARG_TYPES_IDX, i + 1);
                lua_rawseti(state, -2, i + 1);
            }
            lua_setiuservalue(state, -2, 2);

            // p_type is on the stack
            luaL_getmetatable(state, "amoeba.c.ffi.CType");
            lua_setmetatable(state, -2);
            return 1;
        }
    },
    { nullptr, nullptr }
};

static const luaL_Reg ffi_CInterface_lib_lua[] = {
    {
        "make",
        [](lua_State *state) -> int {
            const int ARG_RTYPE_IDX = 1;
            const int ARG_ATYPES_IDX = 2;
            const int ARG_ABI_IDX = 3;
            ffi_type *p_rtype = reinterpret_cast<ffi_type *>(luaL_checkudata(state, ARG_RTYPE_IDX, "amoeba.c.ffi.CType"));
            luaL_checktype(state, ARG_ATYPES_IDX, LUA_TTABLE);
            unsigned int nargs = static_cast<unsigned int>(lua_rawlen(state, ARG_ATYPES_IDX));
            ffi_abi abi = static_cast<ffi_abi>(luaL_optinteger(state, ARG_ABI_IDX, FFI_WIN64));

            ffi_cif *p_cif = reinterpret_cast<ffi_cif *>(lua_newuserdatauv(state, sizeof(ffi_cif), 4));

            ffi_type **pp_atypes = reinterpret_cast<ffi_type **>(lua_newuserdatauv(state, sizeof(ffi_type *) * nargs, 0));
            for(unsigned int i = 0; i < nargs; i++) {
                lua_rawgeti(state, ARG_ATYPES_IDX, i + 1);
                ffi_type *p_atype = reinterpret_cast<ffi_type *>(luaL_testudata(state, -1, "amoeba.c.ffi.CType"));
                if(p_atype == nullptr) {
                    luaL_error(state, "atypes index %d is not a CType", i + 1);
                }
                pp_atypes[i] = p_atype;
                lua_pop(state, 1);
            }
            lua_setiuservalue(state, -2, 1);

            ffi_status result = ffi_prep_cif(p_cif, abi, nargs, p_rtype, pp_atypes);
            if(result != FFI_OK) {
                luaL_error(state, "ffi_prep_cif failed with status %d", result);
            }

            void **pp_avalue = reinterpret_cast<void **>(lua_newuserdatauv(state, sizeof(void *) * nargs * 2, 0));
            for(unsigned int i = 0; i < nargs; i++) {
                pp_avalue[nargs + i] = &pp_avalue[i];
            }
            lua_setiuservalue(state, -2, 2);

            // TODO assert on unsatisfied overalignment requirement (type->alignment)?
            void *p_rvalue = reinterpret_cast<void *>(lua_newuserdatauv(state, p_cif->rtype->size, 0));
            lua_setiuservalue(state, -2, 3);
            (void)p_rvalue;

            // Populate a reference tracking array to tie CType lifetime with CInterface
            lua_newtable(state);
            lua_pushvalue(state, ARG_RTYPE_IDX);
            lua_rawseti(state, -2, 1);
            for(unsigned int i = 0; i < nargs; i++) {
                lua_rawgeti(state, ARG_ATYPES_IDX, i + 1);
                lua_rawseti(state, -2, i + 2);
            }
            lua_setiuservalue(state, -2, 4);

            // p_cif is on the stack
            luaL_getmetatable(state, "amoeba.c.ffi.CInterface");
            lua_setmetatable(state, -2);
            return 1;
        }
    },
    {
        "unsafe_call",
        [](lua_State *state) -> int {
            ffi_cif *p_cif = reinterpret_cast<ffi_cif *>(luaL_checkudata(state, 1, "amoeba.c.ffi.CInterface"));
            void *fp = reinterpret_cast<void *>(luaL_checkinteger(state, 2));
            unsigned int bot_idx = 3; // args start at index 3
            unsigned int top_idx = lua_gettop(state);
            unsigned int nargs = top_idx < bot_idx ? 0 : top_idx - bot_idx + 1;

            lua_getiuservalue(state, 1, 2);
            void **pp_avalue = reinterpret_cast<void **>(lua_touserdata(state, -1));
            lua_pop(state, 1);
            lua_getiuservalue(state, 1, 3);
            void *p_rvalue = lua_touserdata(state, -1);
            lua_pop(state, 1);

            if (nargs != p_cif->nargs) {
                luaL_error(state, "lua table length %d does not match ffi_cif nargs %d", nargs, p_cif->nargs);
            }

            // pp_avalue's latter half needs to point to its first half
            for(unsigned int j = bot_idx; j <= top_idx; j++) {
                unsigned int i = j - bot_idx;
                ffi_type *ty = p_cif->arg_types[i];
                switch(ty->type) {
                    case FFI_TYPE_FLOAT: {
                        pp_avalue[i] = 0; // zero full 64 bits first
                        float tmp = static_cast<float>(luaL_checknumber(state, j));
                        std::memcpy(&pp_avalue[i], &tmp, sizeof(float));
                        break;
                    }
                    case FFI_TYPE_DOUBLE: {
                        double tmp = luaL_checknumber(state, j);
                        std::memcpy(&pp_avalue[i], &tmp, sizeof(double));
                        break;
                    }
                    // case FFI_TYPE_LONGDOUBLE: // not supported among Windows compiler superset
                    // case FFI_TYPE_VOID: // not reachable
                    case FFI_TYPE_INT:
                    case FFI_TYPE_UINT8:
                    case FFI_TYPE_SINT8:
                    case FFI_TYPE_UINT16:
                    case FFI_TYPE_SINT16:
                    case FFI_TYPE_UINT32:
                    case FFI_TYPE_SINT32:
                    case FFI_TYPE_UINT64:
                    case FFI_TYPE_SINT64:
                    case FFI_TYPE_STRUCT: // TODO do we need to point to the struct itself?
                    case FFI_TYPE_POINTER:
                    // case FFI_TYPE_COMPLEX: // not supported among Windows compiler superset
                    default:
                        // we're truncating a 2's complement int64, so we shouldn't need to consider
                        // sign extensions for narrower or equal width types
                        pp_avalue[i] = reinterpret_cast<void *>(luaL_checkinteger(state, j));
                        break;
                }
            }

            ffi_call(p_cif, FFI_FN(fp), p_rvalue, &pp_avalue[nargs]);

            switch(p_cif->rtype->type) {
                case FFI_TYPE_FLOAT: {
                    float tmp;
                    std::memcpy(&tmp, p_rvalue, sizeof(float));
                    lua_pushnumber(state, tmp);
                    return 1;
                }
                case FFI_TYPE_DOUBLE: {
                    double tmp;
                    std::memcpy(&tmp, p_rvalue, sizeof(double));
                    lua_pushnumber(state, tmp);
                    return 1;
                }
                // case FFI_TYPE_LONGDOUBLE: // not supported among Windows compiler superset
                case FFI_TYPE_VOID:
                    return 0;
                case FFI_TYPE_INT:
                    lua_pushinteger(state, *reinterpret_cast<int32_t *>(p_rvalue));
                    return 1;
                case FFI_TYPE_UINT8:
                    lua_pushinteger(state, *reinterpret_cast<uint8_t *>(p_rvalue));
                    return 1;
                case FFI_TYPE_SINT8:
                    lua_pushinteger(state, *reinterpret_cast<int8_t *>(p_rvalue));
                    return 1;
                case FFI_TYPE_UINT16:
                    lua_pushinteger(state, *reinterpret_cast<uint16_t *>(p_rvalue));
                    return 1;
                case FFI_TYPE_SINT16:
                    lua_pushinteger(state, *reinterpret_cast<int16_t *>(p_rvalue));
                    return 1;
                case FFI_TYPE_UINT32:
                    lua_pushinteger(state, *reinterpret_cast<uint32_t *>(p_rvalue));
                    return 1;
                case FFI_TYPE_SINT32:
                    lua_pushinteger(state, *reinterpret_cast<int32_t *>(p_rvalue));
                    return 1;
                case FFI_TYPE_UINT64:
                    lua_pushinteger(state, *reinterpret_cast<uint64_t *>(p_rvalue));
                    return 1;
                case FFI_TYPE_SINT64:
                    lua_pushinteger(state, *reinterpret_cast<int64_t *>(p_rvalue));
                    return 1;
                case FFI_TYPE_STRUCT:
                    // TODO need to investigate struct support
                    lua_pushinteger(state, reinterpret_cast<uint64_t>(p_rvalue));
                    return 1;
                case FFI_TYPE_POINTER:
                    lua_pushinteger(state, *reinterpret_cast<uint64_t *>(p_rvalue));
                    return 1;
                // case FFI_TYPE_COMPLEX: // not supported among Windows compiler superset
                default:
                    // probably untraversable case
                    lua_pushinteger(state, reinterpret_cast<uint64_t>(p_rvalue));
                    return 1;
            }
        }
    },
    { nullptr, nullptr }
};

static const luaL_Reg ffi_lib_lua[] = {
    {
        "get_module_handle",
        [](lua_State *state) -> int {
            const char *module_name = luaL_checkstring(state, 1);

            lua_Integer addr = reinterpret_cast<lua_Integer>(GetModuleHandleW(convert_utf8_string_to_utf16_wstring(module_name).c_str()));

            lua_pushinteger(state, addr);
            return 1;
        }
    },
    {
        "get_proc_address",
        [](lua_State *state) -> int {
            HMODULE h_module = reinterpret_cast<HMODULE>(luaL_checkinteger(state, 1));
            const char *proc_name = luaL_checkstring(state, 2);

            lua_Integer addr = reinterpret_cast<lua_Integer>(GetProcAddress(h_module, proc_name));

            lua_pushinteger(state, addr);
            return 1;
        }
    },
    { nullptr, nullptr }
};

// Memory helpers

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

template<typename T>
static int unsafe_write_lua(lua_State *state) {
    T *addr = reinterpret_cast<T *>(luaL_checkinteger(state, 1));
    if constexpr(std::is_integral_v<T>) {
        T data = static_cast<T>(luaL_checkinteger(state, 2));
        *addr = data;
    } else {
        T data = static_cast<T>(luaL_checknumber(state, 2));
        *addr = data;
    }
    return 0;
}

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
    { "unsafe_write_u8", unsafe_write_lua<uint8_t> },
    { "unsafe_write_u16", unsafe_write_lua<uint16_t> },
    { "unsafe_write_u32", unsafe_write_lua<uint32_t> },
    { "unsafe_write_u64", unsafe_write_lua<uint64_t> },
    { "unsafe_write_i8", unsafe_write_lua<int8_t> },
    { "unsafe_write_i16", unsafe_write_lua<int16_t> },
    { "unsafe_write_i32", unsafe_write_lua<int32_t> },
    { "unsafe_write_i64", unsafe_write_lua<int64_t> },
    { "unsafe_write_f32", unsafe_write_lua<float> },
    { "unsafe_write_f64", unsafe_write_lua<double> },
    {
        "alloc",
        [](lua_State *state) -> int {
            size_t size = luaL_checkinteger(state, 1);
            lua_pushinteger(state, reinterpret_cast<lua_Integer>(std::malloc(size)));
            return 1;
        }
    },
    {
        "unsafe_free",
        [](lua_State *state) -> int {
            void *addr = reinterpret_cast<void *>(luaL_checkinteger(state, 1));
            std::free(addr);
            return 0;
        }
    },
    {
        "unsafe_realloc",
        [](lua_State *state) -> int {
            void *addr = reinterpret_cast<void *>(luaL_checkinteger(state, 1));
            size_t size = luaL_checkinteger(state, 2);
            lua_pushinteger(state, reinterpret_cast<lua_Integer>(std::realloc(addr, size)));
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

// Host process helpers

static const luaL_Reg mew_lib_lua[] = {
    {
        "get_mewdirector",
        [](lua_State *state) -> int {
            lua_pushinteger(state, reinterpret_cast<lua_Integer>(get_p_mewdirector_singleton()));
            return 1;
        }
    },
    {
        "host_alloc",
        [](lua_State *state) -> int {
            size_t size = luaL_checkinteger(state, 1);
            lua_pushinteger(state, reinterpret_cast<lua_Integer>(host_alloc(size)));
            return 1;
        }
    },
    {
        "unsafe_host_free",
        [](lua_State *state) -> int {
            void *addr = reinterpret_cast<void *>(luaL_checkinteger(state, 1));
            host_free(addr);
            return 0;
        }
    },
    {
        "unsafe_host_realloc",
        [](lua_State *state) -> int {
            void *addr = reinterpret_cast<void *>(luaL_checkinteger(state, 1));
            size_t size = luaL_checkinteger(state, 2);
            lua_pushinteger(state, reinterpret_cast<lua_Integer>(host_realloc(addr, size)));
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

        luaL_requiref(state, "amoeba.c.ffi", [](lua_State *state) -> int {
            luaL_newlib(state, ffi_lib_lua);

            // CType
            if(luaL_newmetatable(state, "amoeba.c.ffi.CType")) {
                luaL_setfuncs(state, ffi_CType_lib_lua, 0);
                lua_pushvalue(state, -1);
                lua_setfield(state, -2, "__index");

                lua_pushliteral(state, "metatable access protected");
                lua_setfield(state, -2, "__metatable");
            }
            lua_setfield(state, -2, "CType");

            // CInterface
            if(luaL_newmetatable(state, "amoeba.c.ffi.CInterface")) {
                luaL_setfuncs(state, ffi_CInterface_lib_lua, 0);
                lua_pushvalue(state, -1);
                lua_setfield(state, -2, "__index");

                lua_pushliteral(state, "metatable access protected");
                lua_setfield(state, -2, "__metatable");
            }
            lua_setfield(state, -2, "CInterface");

            // e_status
            lua_newtable(state);
            lua_pushinteger(state, FFI_OK);
            lua_setfield(state, -2, "OK");
            lua_pushinteger(state, FFI_BAD_TYPEDEF);
            lua_setfield(state, -2, "BAD_TYPEDEF");
            lua_pushinteger(state, FFI_BAD_ABI);
            lua_setfield(state, -2, "BAD_ABI");
            lua_pushinteger(state, FFI_BAD_ARGTYPE);
            lua_setfield(state, -2, "BAD_ARGTYPE");
            lua_setfield(state, -2, "e_status");

            // e_abi
            lua_newtable(state);
            lua_pushinteger(state, FFI_WIN64);
            lua_setfield(state, -2, "WIN64");
            lua_pushinteger(state, FFI_GNUW64);
            lua_setfield(state, -2, "GNUW64");
            // lua_pushinteger(state, FFI_DEFAULT_ABI);
            // lua_setfield(state, -2, "DEFAULT_ABI");
            lua_setfield(state, -2, "e_abi");

            // e_type
            auto push_primitive_ctype = [](lua_State *state, ffi_type *p_typein) {
                ffi_type *p_type = reinterpret_cast<ffi_type *>(lua_newuserdatauv(state, sizeof(ffi_type), 0));

                std::memcpy(p_type, p_typein, sizeof(ffi_type));

                // p_type is on the stack
                luaL_getmetatable(state, "amoeba.c.ffi.CType");
                lua_setmetatable(state, -2);
            };
            lua_newtable(state);
            push_primitive_ctype(state, &ffi_type_void);
            lua_setfield(state, -2, "void");
            push_primitive_ctype(state, &ffi_type_uint8);
            lua_setfield(state, -2, "uint8");
            push_primitive_ctype(state, &ffi_type_sint8);
            lua_setfield(state, -2, "sint8");
            push_primitive_ctype(state, &ffi_type_uint16);
            lua_setfield(state, -2, "uint16");
            push_primitive_ctype(state, &ffi_type_sint16);
            lua_setfield(state, -2, "sint16");
            push_primitive_ctype(state, &ffi_type_uint32);
            lua_setfield(state, -2, "uint32");
            push_primitive_ctype(state, &ffi_type_sint32);
            lua_setfield(state, -2, "sint32");
            push_primitive_ctype(state, &ffi_type_uint64);
            lua_setfield(state, -2, "uint64");
            push_primitive_ctype(state, &ffi_type_sint64);
            lua_setfield(state, -2, "sint64");
            push_primitive_ctype(state, &ffi_type_float);
            lua_setfield(state, -2, "float");
            push_primitive_ctype(state, &ffi_type_double);
            lua_setfield(state, -2, "double");
            push_primitive_ctype(state, &ffi_type_pointer);
            lua_setfield(state, -2, "pointer");
            // push_primitive_ctype(state, &ffi_type_longdouble);
            // lua_setfield(state, -2, "longdouble");
            lua_setfield(state, -2, "e_type");

            return 1;
        }, 0);
        lua_setfield(state, -2, "ffi");

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

        return 1;
    }, 0);
    lua_pop(this->state, 1);

    // https://stackoverflow.com/questions/4125971/setting-the-global-lua-path-variable-from-c-c
    lua_getglobal(this->state, "package"); // push
    // Set the module search path to be relative to our dll, not the host cwd (LUA_PATH_DEFAULT)
    std::string path;
    std::filesystem::path dll_parent_dir = get_module_file_path(reinterpret_cast<HMODULE>(&__ImageBase)).parent_path();
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
