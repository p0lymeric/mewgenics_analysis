cmake_minimum_required(VERSION 3.15)

project(mewgenics_analysis)

# Set global compile options
if(MSVC)
    # append "-- /p:CL_MPcount=<n>" to "cmake --build" to control TU-level parallelism
    add_compile_options(/MP)
endif()

# Register our CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/cmake/modules")

# Read third-party library CMakeLists
add_subdirectory(lib/minhook EXCLUDE_FROM_ALL)
add_subdirectory(lib/lz4/build/cmake EXCLUDE_FROM_ALL)
add_subdirectory(lib/cmake/detours EXCLUDE_FROM_ALL)
add_subdirectory(lib/cmake/SDL EXCLUDE_FROM_ALL)
add_subdirectory(lib/cmake/imgui EXCLUDE_FROM_ALL)
add_subdirectory(lib/Mewgenics EXCLUDE_FROM_ALL)
add_subdirectory(lib/sqlite3 EXCLUDE_FROM_ALL)
add_subdirectory(lib/cmake/libtomcrypt EXCLUDE_FROM_ALL)
add_subdirectory(lib/cmake/mewjector EXCLUDE_FROM_ALL)
add_subdirectory(lib/lua EXCLUDE_FROM_ALL)
add_subdirectory(lib/cmake/libffi EXCLUDE_FROM_ALL)

# Read our CMakeLists
add_subdirectory(amoeba)
add_subdirectory(cosmic_ooze)
# add_subdirectory(tests)
