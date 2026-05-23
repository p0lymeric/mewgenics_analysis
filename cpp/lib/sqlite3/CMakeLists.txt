project(sqlite3)

# Set path anchors
set(SQLITE3_AMALGAMATION_DIR third_party/sqlite-amalgamation-3380500)

# List build targets
add_library(${PROJECT_NAME} STATIC
    ${SQLITE3_AMALGAMATION_DIR}/sqlite3.c
    #${SQLITE3_AMALGAMATION_DIR}/shell.c
)

# List include directories
target_include_directories(${PROJECT_NAME} PUBLIC
    ${SQLITE3_AMALGAMATION_DIR}
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
        target_compile_options(${PROJECT_NAME} PRIVATE /W4)
    endif()
elseif(MINGW)
    # not clean with GCC/Clang in GCC mode, suppress
    target_compile_options(${PROJECT_NAME} PRIVATE -w)
endif()
