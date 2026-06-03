#include "lua.hpp"

#include <functional>
#include <string>

// Lua wrapper
//
// polymeric 2026

class LuaWrapper {
public:
    using PrintFn = std::function<void(std::string line)>;

    // conventional way to key a datum in the global registry
    // https://www.lua.org/pil/27.3.1.html
    static inline const void *LuaWrapperKey = nullptr;

    lua_State *state = nullptr;
    PrintFn print_stdout_callback;
    bool log_stdio = false;

    LuaWrapper();
    ~LuaWrapper();

    void set_print_stdout_callback(PrintFn cb);
    void reset();

private:
    void init();
};
