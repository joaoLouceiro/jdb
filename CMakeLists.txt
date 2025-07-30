# set(CMAKE_EXPORT_COMPILE_COMMANDS ON) 
cmake_minimum_required(VERSION 3.19)

# Naming the project and enabling C++ support
project("jdb" LANGUAGES CXX)

# Added after vcpkg
# find_package looks for installed libraries
# libedit was added as a dependency in vcpkg.json
find_package(PkgConfig REQUIRED)
pkg_check_modules(libedit REQUIRED IMPORTED_TARGET libedit)

# Enable CMakes builtin testing and add a BUILD_TESTING variable that users can set to select wheter to build the tests when configuring the project
include(CTest)

# Load the CMakeLists.txt files in both /src and /tools
add_subdirectory("src")
add_subdirectory("tools")

# Load the CMakeLists.txt files in /test id the user doesn't say otherwise, since BUILD_TESTING is TRUE by default
if(BUILD_TESTING)
    find_package(Catch2 CONFIG REQUIRED)
    add_subdirectory("test")
endif()

