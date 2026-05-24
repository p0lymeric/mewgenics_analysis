project(amoeba)

# List build targets
add_library(${PROJECT_NAME} SHARED
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

# List dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE
    # minhook
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

# List include directories
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# List definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
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
#target_link_libraries(${PROJECT_NAME} PRIVATE enable_asan)
#target_link_libraries(dear_imgui PRIVATE enable_asan) # instrumented to avoid linker errors with imgui_stdlib.cpp
#install(FILES ${ASAN_DYNAMIC_DLLS} DESTINATION bin)

# Set required flags
if(MSVC)
elseif(MINGW)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # Statically link libc++ (static-libstdc++ flag also covers libc++)
        target_link_options(${PROJECT_NAME} PRIVATE -static-libstdc++)
        # Generate a PDB
        target_link_options(${PROJECT_NAME} PRIVATE -Wl,--pdb=)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # Statically link libgcc/libstdc++/libwinpthread
        target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++ -static -lwinpthread -dynamic)
    endif()
    # Don't prefix the hook dll with "lib" in its filename
    set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
endif()

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
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
if(MSVC)
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION bin OPTIONAL)
elseif(MINGW)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        install(FILES "$<TARGET_FILE_DIR:${PROJECT_NAME}>/$<TARGET_FILE_BASE_NAME:${PROJECT_NAME}>.pdb" DESTINATION bin OPTIONAL)
    endif()
endif()
