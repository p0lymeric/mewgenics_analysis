cmake_minimum_required(VERSION 3.15)

project(libtomcrypt)

# Set language mode
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
# possibly does nothing with MSVC
set(CMAKE_C_EXTENSIONS OFF)

set(LIBTOMCRYPT_DIR ../../libtomcrypt)

if(MSVC)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_compile_options(/W4)
    else()
        add_compile_options(/W4)
    endif()
elseif(MINGW)
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Define build targets
add_library(libtomcrypt STATIC
    ${LIBTOMCRYPT_DIR}/src/hashes/sha2/sha256.c
)

target_include_directories(libtomcrypt PUBLIC
    ${LIBTOMCRYPT_DIR}/src/headers
)

target_compile_definitions(libtomcrypt PRIVATE
    LTC_NO_TEST
    ARGTYPE=4
)

target_compile_definitions(libtomcrypt PUBLIC
    LTC_NO_PROTOTYPES
)
