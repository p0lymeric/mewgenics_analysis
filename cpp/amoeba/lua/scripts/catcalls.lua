-- Test script: Catcalls (not the creepy kind)
-- polymeric 2026

local amoeba = require("amoeba")

local fp_get_window = amoeba.c.ffi.get_proc_address(0, "SDL_GL_GetCurrentWindow")
local cfun_get_window = amoeba.utils.ffi.CFunction:new(
    amoeba.utils.ffi.e_type.pointer,
    {}
)

local fp_minimize = amoeba.c.ffi.get_proc_address(0, "SDL_MinimizeWindow")
local cfun_minimize = amoeba.utils.ffi.CFunction:new(
    amoeba.utils.ffi.e_type.void,
    { amoeba.utils.ffi.e_type.pointer }
)

local fp_sin = amoeba.c.ffi.get_proc_address(0, "SDL_sin")
local cfun_sin = amoeba.utils.ffi.CFunction:new(
    amoeba.utils.ffi.e_type.double,
    { amoeba.utils.ffi.e_type.double }
)

local fp_cosf = amoeba.c.ffi.get_proc_address(0, "SDL_cosf")
local cfun_cosf = amoeba.utils.ffi.CFunction:new(
    amoeba.utils.ffi.e_type.float,
    { amoeba.utils.ffi.e_type.float }
)

local fp_messageboxa = amoeba.c.ffi.get_proc_address(amoeba.c.ffi.get_module_handle("user32.dll"), "MessageBoxA")
local cfun_messageboxa = amoeba.utils.ffi.CFunction:new(
    amoeba.utils.ffi.e_type.sint32,
    { amoeba.utils.ffi.e_type.pointer, amoeba.utils.ffi.e_type.pointer, amoeba.utils.ffi.e_type.pointer, amoeba.utils.ffi.e_type.uint32 }
)

---@param str string
local cstring = function(str)
    local strlen = str:len()
    local p_cstr = amoeba.c.mem.alloc(strlen + 1)
    for i = 1, strlen do
        amoeba.c.mem.unsafe_write_u8(p_cstr + i - 1, str:byte(i))
    end
    amoeba.c.mem.unsafe_write_u8(p_cstr + strlen, 0)
    return p_cstr
end

local sinpi = cfun_sin:unsafe_call(fp_sin, math.pi)

local cospi = cfun_cosf:unsafe_call(fp_cosf, math.pi)

local p_text = cstring(string.format("mrrow (fun math facts)\nsin(pi)=%f\ncos(pi)=%f", sinpi, cospi))
local p_caption = cstring("meow meow")

local p_window = cfun_get_window:unsafe_call(fp_get_window)

cfun_minimize:unsafe_call(fp_minimize, p_window)
local ret_val = cfun_messageboxa:unsafe_call(fp_messageboxa, 0, p_text, p_caption, 0)

print(string.format("MessageBoxA returned %d", ret_val))

amoeba.c.mem.unsafe_free(p_text)
amoeba.c.mem.unsafe_free(p_caption)
