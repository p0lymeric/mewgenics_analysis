cmake_minimum_required(VERSION 3.15)

project(amoeba)

# Build target names
set(HOOK_NAME amoeba)

# Set language mode (C++)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# possibly does nothing with MSVC; permissive- is set by std:c++20
set(CMAKE_CXX_EXTENSIONS OFF)

# Set language mode (C)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
# possibly does nothing with MSVC
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4)
elseif(MINGW)
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Define build targets
add_library(${HOOK_NAME} SHARED
    amoeba.cpp
    amoeba.def
    amoeba_sql_logger.cpp
    amoeba_imgui.cpp
    amoeba_flappy_cat.cpp
    utilities/transaction_logger.cpp
    utilities/sqlite3_conn_wrapper.cpp
    utilities/mewjector_support.c
    utilities/imgui_support.cpp
    ffi/cat_factory.cpp
    ffi/experimental.cpp
)

target_include_directories(${HOOK_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(${HOOK_NAME} PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

if(MSVC)
elseif(MINGW)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # Statically link libc++ (static-libstdc++ flag also covers libc++)
        target_link_options(${HOOK_NAME} PRIVATE -static-libstdc++)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # Statically link libgcc/libstdc++/libwinpthread
        target_link_options(${HOOK_NAME} PRIVATE -static-libgcc -static-libstdc++ -static -lwinpthread -dynamic)
    endif()
    # Don't prefix the hook dll with "lib" in its filename
    set_target_properties(${HOOK_NAME} PROPERTIES PREFIX "")
endif()

target_link_libraries(${HOOK_NAME} PRIVATE
    Detours
    lz4
    dear_imgui
    SDL3::Headers
    # Instead of building SDL3, we use exported symbols from the exe (Mewgenics_exports) instead
    Mewgenics_exports
    sqlite3
    libtomcrypt
    mewjector
)

# Notes for using ASAN:
# 1. Must load (lib)clang_rt.asan_dynamic-x86_64.dll when Mewgenics.exe is launched.
#    ASAN needs to witness all allocations in the process's life. Loading during mid-life will lead to a report and crash.
# 2. To load amoeba and ASAN together from the start, best to use cosmic_ooze's Detours-based injection method.
#    Place amoeba.dll, (lib)clang_rt.asan_dynamic-x86_64.dll, and cosmic_ooze.exe in the same directory as
#    Mewgenics.exe and start Mewgenics with cosmic_ooze.exe.
#    (lib)clang_rt.asan_dynamic-x86_64.dll will load via transitive dependency to amoeba.dll.
# 3. amoeba.dll may still be ejected and reloaded via cosmic_ooze's LoadLibrary injection method after the initial load.
# 4. All library code can be included except hooking libraries, but we only focus on our code.
# 5. MSVC's ASAN will significantly increase Mewgenics load times. ClangCL/MinGW-Clang64's ASAN are much more performant.
# 6. Get ready to pounce on that juicy memory bug!

#include(Asan)
#target_link_libraries(${HOOK_NAME} PRIVATE enable_asan)
#target_link_libraries(dear_imgui PRIVATE enable_asan) # instrumented to avoid linker errors with imgui_stdlib.cpp
#install(FILES ${ASAN_DYNAMIC_DLLS} TYPE BIN)

install(TARGETS ${TARGET_NAME} ${HOOK_NAME} RUNTIME DESTINATION bin)

if(MSVC)
    install(FILES $<TARGET_PDB_FILE:${HOOK_NAME}> TYPE BIN OPTIONAL)
endif()
