cmake_minimum_required(VERSION 3.15)

project(Detours)

# Set language mode
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# possibly does nothing with MSVC; permissive- is set by std:c++20
# set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    # Detours 4.0.1 needs permissive C++ flavour for compile
    add_compile_options(/W4 /permissive)
endif()

set(DETOURS_DIR ../../detours)

add_library(Detours STATIC
    ${DETOURS_DIR}/src/detours.cpp
    ${DETOURS_DIR}/src/modules.cpp
    ${DETOURS_DIR}/src/disasm.cpp
    #${DETOURS_DIR}/src/image.cpp
    ${DETOURS_DIR}/src/creatwth.cpp
    #${DETOURS_DIR}/src/disolx86.cpp
    #${DETOURS_DIR}/src/disolx64.cpp
    #${DETOURS_DIR}/src/disolia64.cpp
    #${DETOURS_DIR}/src/disolarm.cpp
    #${DETOURS_DIR}/src/disolarm64.cpp
)

target_include_directories(Detours PUBLIC
    ${DETOURS_DIR}/src
)
