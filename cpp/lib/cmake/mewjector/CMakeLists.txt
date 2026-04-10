cmake_minimum_required(VERSION 3.15)

project(mewjector)

set(MEWJECTOR_DIR ../../mewjector)

add_library(mewjector INTERFACE)

target_include_directories(mewjector INTERFACE
    ${MEWJECTOR_DIR}/
)
