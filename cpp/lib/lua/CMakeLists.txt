project(lua)

# Set path anchors
set(LUA_DIR third_party/lua-5.5.0)

# List build targets
add_library(${PROJECT_NAME} STATIC
    # CORE_O
    ${LUA_DIR}/src/lapi.c
    ${LUA_DIR}/src/lcode.c
    ${LUA_DIR}/src/lctype.c
    ${LUA_DIR}/src/ldebug.c
    ${LUA_DIR}/src/ldo.c
    ${LUA_DIR}/src/ldump.c
    ${LUA_DIR}/src/lfunc.c
    ${LUA_DIR}/src/lgc.c
    ${LUA_DIR}/src/llex.c
    ${LUA_DIR}/src/lmem.c
    ${LUA_DIR}/src/lobject.c
    ${LUA_DIR}/src/lopcodes.c
    ${LUA_DIR}/src/lparser.c
    ${LUA_DIR}/src/lstate.c
    ${LUA_DIR}/src/lstring.c
    ${LUA_DIR}/src/ltable.c
    ${LUA_DIR}/src/ltm.c
    ${LUA_DIR}/src/lundump.c
    ${LUA_DIR}/src/lvm.c
    ${LUA_DIR}/src/lzio.c
    # LIB_O
    ${LUA_DIR}/src/lauxlib.c
    ${LUA_DIR}/src/lbaselib.c
    ${LUA_DIR}/src/lcorolib.c
    ${LUA_DIR}/src/ldblib.c
    ${LUA_DIR}/src/liolib.c
    ${LUA_DIR}/src/lmathlib.c
    ${LUA_DIR}/src/loadlib.c
    ${LUA_DIR}/src/loslib.c
    ${LUA_DIR}/src/lstrlib.c
    ${LUA_DIR}/src/ltablib.c
    ${LUA_DIR}/src/lutf8lib.c
    ${LUA_DIR}/src/linit.c
)

# List include directories
target_include_directories(${PROJECT_NAME} PUBLIC
    ${LUA_DIR}/src
)

# Set language modes
set_target_properties(${PROJECT_NAME} PROPERTIES
    C_STANDARD 17
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)

# Set warnings
if(MSVC)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # not clean with Clang in MSVC mode, suppress
        target_compile_options(${PROJECT_NAME} PRIVATE /W0)
    else()
        # not clean with MSVC, suppress
        target_compile_options(${PROJECT_NAME} PRIVATE /W0)
    endif()
elseif(MINGW)
    # not clean with GCC/Clang in GCC mode, suppress
    target_compile_options(${PROJECT_NAME} PRIVATE -w)
endif()
