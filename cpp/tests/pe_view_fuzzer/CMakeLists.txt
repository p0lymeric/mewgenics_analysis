# libFuzzer tests only work on MSVC without much struggle
# (VS only ships MT libs with ClangCL, MinGW-Clang64 does not support libFuzzer at all)
if((MSVC AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang") OR MINGW)
    return()
endif()

project(pe_view_fuzzer)

# List build targets
add_executable(${PROJECT_NAME}
    fuzzer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../amoeba/utilities/pe_view.cpp
)

# List include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../amoeba
)

# Set required flags
include(Asan)
# Linker errors emerge unless /fsanitize=address is set. Not sure about true cause.
target_link_libraries(${PROJECT_NAME} PRIVATE enable_asan)
install(FILES ${ASAN_DYNAMIC_DLLS} DESTINATION tests)
target_compile_options(${PROJECT_NAME} PRIVATE /fsanitize=fuzzer)
# Avoid generating warnings
target_link_options(${PROJECT_NAME} PRIVATE /INCREMENTAL:NO)

# Set language modes
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    C_STANDARD 17
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)

# Set warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
elseif(MINGW)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set install paths
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION tests)
if(MSVC)
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION tests OPTIONAL)
endif()
