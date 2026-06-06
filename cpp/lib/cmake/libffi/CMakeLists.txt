project(libffi)

# Set assembler flavour
if(MSVC)
    enable_language(ASM_MASM)
elseif(MINGW)
    enable_language(ASM)
endif()

# Set path anchors
set(LIBFFI_DIR ../../libffi)

# List build targets

add_library(${PROJECT_NAME} STATIC
    # libffi_la_SOURCES
    ${LIBFFI_DIR}/src/prep_cif.c
    ${LIBFFI_DIR}/src/types.c
    ${LIBFFI_DIR}/src/raw_api.c
    ${LIBFFI_DIR}/src/java_raw_api.c
    ${LIBFFI_DIR}/src/closures.c
    ${LIBFFI_DIR}/src/tramp.c

    # libffi_la_SOURCES with FFI_DEBUG
    # ${LIBFFI_DIR}/src/debug.c

    # configure.host (Windows 64b)
    ${LIBFFI_DIR}/src/x86/ffiw64.c
)
if(MSVC)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/win64_intel.asm
        COMMAND ${CMAKE_C_COMPILER} /EP /P
                /I ${CMAKE_CURRENT_SOURCE_DIR}/${LIBFFI_DIR}/include
                /I ${CMAKE_CURRENT_SOURCE_DIR}/${LIBFFI_DIR}/src/x86
                /I ${CMAKE_CURRENT_SOURCE_DIR}/autoconf
                /DFFI_STATIC_BUILD
                /Fi${CMAKE_CURRENT_BINARY_DIR}/win64_intel.asm
                ${CMAKE_CURRENT_SOURCE_DIR}/${LIBFFI_DIR}/src/x86/win64_intel.S
        DEPENDS ${LIBFFI_DIR}/src/x86/win64_intel.S
    )
    target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/win64_intel.asm)
elseif(MINGW)
    target_sources(${PROJECT_NAME} PRIVATE ${LIBFFI_DIR}/src/x86/win64.S)
endif()

# List include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${LIBFFI_DIR}/include
)
target_include_directories(${PROJECT_NAME} PUBLIC
    # ffi.h and ffi_target.h are meant to be public
    ${LIBFFI_DIR}/src/x86
    autoconf
)

# List definitions
target_compile_definitions(${PROJECT_NAME} PUBLIC
    FFI_STATIC_BUILD
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
        # not clean with MSVC, suppress
        target_compile_options(${PROJECT_NAME} PRIVATE /W0)
    endif()
elseif(MINGW)
    # not clean with GCC/Clang in GCC mode, suppress
    target_compile_options(${PROJECT_NAME} PRIVATE -w)
endif()
