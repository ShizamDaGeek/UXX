cmake_minimum_required(VERSION 3.31.10)
project(UXX C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_LONG_PATHS_SUPPORT ON)



# Add compiler definitions
add_compile_definitions(GLM_ENABLE_EXPERIMENTAL)



# Include all libraries
include_directories(
    libs/GLAD/include
    libs/GLM
    libs/STB
)



# Add GLAD
add_library(glad STATIC libs/GLAD/src/glad.c)
# Add GLFW
add_subdirectory(libs/GLFW)



# Find packages
find_package(OpenGL REQUIRED)



# Adding all STB libaries
add_library(stb_image STATIC src/stb_image.cpp)
target_include_directories(stb_image PUBLIC libs/STB)
add_library(stb_truetype STATIC src/stb_truetype.cpp)
target_include_directories(stb_truetype PUBLIC libs/STB)



# Get all .cpp files in the src directory
file(GLOB_RECURSE SRC_FILES "src/*.cpp")
add_executable(UXX ${SRC_FILES})



# Link all the library and shit
target_link_libraries(UXX PRIVATE
    glfw
    glad
    OpenGL::GL
    stb_image
    stb_truetype
)
