cmake_minimum_required(VERSION 3.10)

project(combined VERSION 1.0)

add_definitions('-Wall')

include(GNUInstallDirs)

find_package(MPI REQUIRED)

include_directories(${MPI_INCLUDE_PATH})

add_subdirectory (src)
target_include_directories(${PROJECT_NAME} PUBLIC include PRIVATE src)

enable_testing ()
add_subdirectory (tests)

add_subdirectory (example)
