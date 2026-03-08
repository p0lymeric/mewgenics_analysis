cmake_minimum_required(VERSION 3.15)

project(mewgenics_analysis)

# Read third-party library CMakeLists
add_subdirectory(lib/minhook EXCLUDE_FROM_ALL)
add_subdirectory(lib/lz4/build/cmake EXCLUDE_FROM_ALL)

# Read our CMakeLists
add_subdirectory(amoeba)
