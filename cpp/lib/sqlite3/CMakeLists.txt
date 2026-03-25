cmake_minimum_required(VERSION 3.15)

project(sqlite3)

# Set language mode
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
# possibly does nothing with MSVC
set(CMAKE_C_EXTENSIONS OFF)

set(SQLITE3_AMALGAMATION_DIR third_party/sqlite-amalgamation-3380500)

if(MSVC)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # not clean with Clang in MSVC mode, suppress
        add_compile_options(/W0)
    else()
        # clean with MSVC
        add_compile_options(/W4)
    endif()
elseif(MINGW)
    # not clean with GCC/Clang in GCC mode, suppress
    add_compile_options(-w)
endif()

# Define build targets
add_library(sqlite3 STATIC
    ${SQLITE3_AMALGAMATION_DIR}/sqlite3.c
    #${SQLITE3_AMALGAMATION_DIR}/shell.c
)

target_include_directories(sqlite3 PUBLIC
    ${SQLITE3_AMALGAMATION_DIR}
)
