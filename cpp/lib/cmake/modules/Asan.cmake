# Notes for using ASAN:
# 1. MSVC and ClangCL support ASAN without LSAN. MinGW-Clang64 additionally supports UBSAN.

add_library(enable_asan INTERFACE)
if(MSVC)
    target_compile_options(enable_asan INTERFACE /fsanitize=address)

    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} -print-resource-dir
            OUTPUT_VARIABLE CLANG_RESOURCE_DIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        cmake_path(NORMAL_PATH CLANG_RESOURCE_DIR)
        target_link_libraries(enable_asan INTERFACE
            ${CLANG_RESOURCE_DIR}/lib/windows/clang_rt.asan_dynamic-x86_64.lib
            ${CLANG_RESOURCE_DIR}/lib/windows/clang_rt.asan_dynamic_runtime_thunk-x86_64.lib
        )
        set(ASAN_DYNAMIC_DLLS
            ${CLANG_RESOURCE_DIR}/lib/windows/clang_rt.asan_dynamic-x86_64.dll
        )
    else()
        cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH COMPILER_BIN_DIR)
        set(ASAN_DYNAMIC_DLLS
            ${COMPILER_BIN_DIR}/clang_rt.asan_dynamic-x86_64.dll
        )
    endif()
elseif(MINGW)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(enable_asan INTERFACE -fsanitize=address,undefined)
        target_link_options(enable_asan INTERFACE -fsanitize=address,undefined)
        cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH COMPILER_BIN_DIR)
        set(ASAN_DYNAMIC_DLLS
            ${COMPILER_BIN_DIR}/libclang_rt.asan_dynamic-x86_64.dll
            ${COMPILER_BIN_DIR}/libc++.dll
        )
    else()
        message(FATAL_ERROR
            "When compiling with MinGW, ASAN is only supported with the Clang64 environment."
        )
    endif()
endif()

# Clang/ClangCL and MSVC define different presence macros, so we define our own
target_compile_definitions(enable_asan INTERFACE ASAN_PRESENT)
