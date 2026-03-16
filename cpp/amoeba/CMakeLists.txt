cmake_minimum_required(VERSION 3.15)

project(amoeba)

# Build target names
set(INJECTOR_NAME cosmic_ooze)
set(HOOK_NAME amoeba)

# Set language mode
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# possibly does nothing with MSVC; permissive- is set by std:c++20
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4)
elseif(MINGW)
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Define build targets
add_executable(${INJECTOR_NAME} WIN32
    cosmic_ooze.cpp
)
add_library(${HOOK_NAME} SHARED
    amoeba.cpp
    amoeba.def
)

if(MSVC)
elseif(MINGW)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # Statically link libc++ (static-libstdc++ flag also covers libc++)
        target_link_options(${INJECTOR_NAME} PRIVATE -static-libstdc++)
        target_link_options(${HOOK_NAME} PRIVATE -static-libstdc++)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # Statically link libgcc/libstdc++/libwinpthread
        target_link_options(${INJECTOR_NAME} PRIVATE -static-libgcc -static-libstdc++ -static -lwinpthread -dynamic)
        target_link_options(${HOOK_NAME} PRIVATE -static-libgcc -static-libstdc++ -static -lwinpthread -dynamic)
    endif()
    # Set -municode so that gcc/clang will use wWinMain as an entry point
    target_link_options(${INJECTOR_NAME} PRIVATE -municode)
    # Don't prefix amoeba with "lib" in its filename
    set_target_properties(${HOOK_NAME} PROPERTIES PREFIX "")
endif()

target_link_libraries(${INJECTOR_NAME} PRIVATE Detours)
# Instead of building SDL3, we use exported symbols from the exe (Mewgenics_exports) instead
target_link_libraries(${HOOK_NAME} PRIVATE Detours lz4 dear_imgui SDL3::Headers Mewgenics_exports)

install(TARGETS ${TARGET_NAME} ${INJECTOR_NAME} RUNTIME DESTINATION bin)
install(TARGETS ${TARGET_NAME} ${HOOK_NAME} RUNTIME DESTINATION bin)

if(MSVC)
    install(FILES $<TARGET_PDB_FILE:${INJECTOR_NAME}> TYPE BIN OPTIONAL)
    install(FILES $<TARGET_PDB_FILE:${HOOK_NAME}> TYPE BIN OPTIONAL)
endif()
