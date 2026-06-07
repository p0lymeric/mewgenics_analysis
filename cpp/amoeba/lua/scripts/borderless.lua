-- Test script: Borderless "fullscreen"
-- polymeric 2026

-- Executes the borderless fullscreen reset routine from Mewgenics, but with resolution off by
-- one to prevent display drivers from promoting the draw surface to exclusive fullscreen.

-- Mewgenics 1.1.21039 (SHA-256 c3a41e436a93fa58cd386ec46dad5c2a6f21a583d33c3a57a15a2604c726439e)
-- Canonical VA: 1409a9750
-- Signature: 48 89 5C 24 10 48 89 74 24 18 55 57 41 56 48 8B EC 48 83 EC 60 48 8B D9 32 C9 48 8D B3 E4 0D 00 00

local uffi = require("amoeba.utils.ffi")
local cmem = require("amoeba.c.mem")
local cffi = require("amoeba.c.ffi")

-- SDL_Window * SDL_GL_GetCurrentWindow(void);
local cfun_sdl_gl_get_current_window = uffi.CFunction:new(
    uffi.e_type.pointer,
    {}
)

-- SDL_DisplayID SDL_GetDisplayForWindow(SDL_Window *window);
local cfun_sdl_get_display_for_window = uffi.CFunction:new(
    uffi.e_type.uint32,
    { uffi.e_type.pointer }
)

-- const SDL_DisplayMode * SDL_GetDesktopDisplayMode(SDL_DisplayID displayID);
local cfun_sdl_get_desktop_display_mode = uffi.CFunction:new(
    uffi.e_type.pointer,
    { uffi.e_type.uint32 }
)

-- bool SDL_SetWindowFullscreen(SDL_Window *window, bool fullscreen);
local cfun_sdl_set_window_fullscreen = uffi.CFunction:new(
    uffi.e_type.uint32,
    { uffi.e_type.pointer, uffi.e_type.uint32 }
)

-- bool SDL_SetWindowSize(SDL_Window *window, int w, int h);
local cfun_sdl_set_window_size = uffi.CFunction:new(
    uffi.e_type.uint32,
    { uffi.e_type.pointer, uffi.e_type.sint32, uffi.e_type.sint32 }
)

-- bool SDL_SetWindowPosition(SDL_Window *window, int x, int y);
local cfun_sdl_set_window_position = uffi.CFunction:new(
    uffi.e_type.uint32,
    { uffi.e_type.pointer, uffi.e_type.sint32, uffi.e_type.sint32 }
)

-- bool SDL_SetWindowResizable(SDL_Window *window, bool resizable);
local cfun_sdl_set_window_resizable = uffi.CFunction:new(
    uffi.e_type.uint32,
    { uffi.e_type.pointer, uffi.e_type.uint32 }
)

-- bool SDL_SetWindowBordered(SDL_Window *window, bool bordered);
local cfun_sdl_set_window_bordered = uffi.CFunction:new(
    uffi.e_type.uint32,
    { uffi.e_type.pointer, uffi.e_type.uint32 }
)

-- bool SDL_RaiseWindow(SDL_Window *window);
local cfun_sdl_raise_window = uffi.CFunction:new(
    uffi.e_type.uint32,
    { uffi.e_type.pointer }
)

local fp_sdl_gl_get_current_window = cffi.get_proc_address(0, "SDL_GL_GetCurrentWindow")
local fp_sdl_get_display_for_window = cffi.get_proc_address(0, "SDL_GetDisplayForWindow")
local fp_sdl_get_desktop_display_mode = cffi.get_proc_address(0, "SDL_GetDesktopDisplayMode")
local fp_sdl_set_window_fullscreen = cffi.get_proc_address(0, "SDL_SetWindowFullscreen")
local fp_sdl_set_window_size = cffi.get_proc_address(0, "SDL_SetWindowSize")
local fp_sdl_set_window_position = cffi.get_proc_address(0, "SDL_SetWindowPosition")
local fp_sdl_set_window_resizable = cffi.get_proc_address(0, "SDL_SetWindowResizable")
local fp_sdl_set_window_bordered = cffi.get_proc_address(0, "SDL_SetWindowBordered")
local fp_sdl_raise_window = cffi.get_proc_address(0, "SDL_RaiseWindow")

local p_window = cfun_sdl_gl_get_current_window:unsafe_call(fp_sdl_gl_get_current_window)
print(string.format("p_window = %x", p_window))

local display_id = cfun_sdl_get_display_for_window:unsafe_call(fp_sdl_get_display_for_window, p_window)
print(string.format("display_id = %d", display_id))

local p_mode = cfun_sdl_get_desktop_display_mode:unsafe_call(fp_sdl_get_desktop_display_mode, display_id)
local w, h
if p_mode == 0 then
    -- Mewgenics appears to set a fallback resolution of 640x480 on SDL_GetDesktopDisplayMode failure
    w = 640
    h = 480
else
    w = cmem.unsafe_read_i32(p_mode + 0x8)
    h = cmem.unsafe_read_i32(p_mode + 0xc)
end
print(string.format("screen resolution: %d x %d", w, h))
print(string.format("screen resolution with borderless: %d x %d", w, h + 1))

cfun_sdl_set_window_fullscreen:unsafe_call(fp_sdl_set_window_fullscreen, p_window, 0)
cfun_sdl_set_window_size:unsafe_call(fp_sdl_set_window_size, p_window, w, h + 1)
local SDL_WINDOWPOS_CENTERED = 0x2FFF0000
cfun_sdl_set_window_position:unsafe_call(fp_sdl_set_window_position, p_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED)
cfun_sdl_set_window_resizable:unsafe_call(fp_sdl_set_window_resizable, p_window, 0)
cfun_sdl_set_window_bordered:unsafe_call(fp_sdl_set_window_bordered, p_window, 0)
cfun_sdl_raise_window:unsafe_call(fp_sdl_raise_window, p_window)

print("applied borderless fullscreen")
