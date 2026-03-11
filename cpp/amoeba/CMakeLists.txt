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

if (MSVC)
    add_compile_options(/W4)
endif()

# Define build targets
add_executable(${INJECTOR_NAME} WIN32
    cosmic_ooze.cpp
)
add_library(${HOOK_NAME} SHARED
    amoeba.cpp
    amoeba.def
)

target_link_libraries(${INJECTOR_NAME} PRIVATE Detours)
target_link_libraries(${HOOK_NAME} PRIVATE minhook Detours lz4)

install(TARGETS ${TARGET_NAME} ${INJECTOR_NAME} RUNTIME DESTINATION bin)
install(TARGETS ${TARGET_NAME} ${HOOK_NAME} RUNTIME DESTINATION bin)

if(MSVC)
    install(FILES $<TARGET_PDB_FILE:${INJECTOR_NAME}> TYPE BIN OPTIONAL)
    install(FILES $<TARGET_PDB_FILE:${HOOK_NAME}> TYPE BIN OPTIONAL)
endif()
