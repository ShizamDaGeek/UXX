cmake_minimum_required(VERSION 3.31.11)
project(UXX C CXX)



# Add compiler definitions
add_compile_definitions(GLM_ENABLE_EXPERIMENTAL)



# Include all libraries
include_directories(
    third_party_libs/GLAD/include
    third_party_libs/GLM
    third_party_libs/STB
    third_party_libs/FREETYPE
)



# Add GLAD
add_library(glad STATIC third_party_libs/GLAD/src/glad.c)
# Add GLFW
add_subdirectory(third_party_libs/GLFW)
# Add FREETYPE
set(CMAKE_WARN_DEPRECATED OFF)
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON CACHE BOOL "" FORCE)
add_subdirectory(third_party_libs/FREETYPE)
set(CMAKE_WARN_DEPRECATED ON)


# Find packages
find_package(OpenGL REQUIRED)



# Add STB_IMAGE as a static library
add_library(stb_image STATIC src/stb_image.cpp)
target_include_directories(stb_image PUBLIC libs/STB)



# Get all .cpp files in the src directory
file(GLOB_RECURSE SRC_FILES "src/*.cpp")
add_executable(UXX ${SRC_FILES})


# have the c++ files look inside of include outside src not vice versa
target_include_directories(UXX PUBLIC include)

# Link all the library
target_link_libraries(UXX PRIVATE
    glfw
    glad
    OpenGL::GL
    stb_image
    freetype
)

if(TARGET freetype)
    target_compile_options(freetype PRIVATE -Wno-deprecated-declarations)
endif()
