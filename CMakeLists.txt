# ===[CMAKE setup]===
cmake_minimum_required(VERSION 3.31.11)
project(UXX VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(UXX_BUILD_DEMO "Build the UXX demo executable" ON)
option(UXX_BUILD_GLFW_BACKEND "Build the GLFW window backend" ON)

# Needed for GLM extensions like glm::radians on some setups
add_compile_definitions(GLM_ENABLE_EXPERIMENTAL)



# ===[Third-party deps]===
add_library(glad STATIC third_party_libs/GLAD/src/glad.c)
target_include_directories(glad PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party_libs/GLAD/include>
    $<INSTALL_INTERFACE:include> # Relative path used once installed
)

if(UXX_BUILD_GLFW_BACKEND)
    add_subdirectory(third_party_libs/GLFW)
endif()

# Stops warnings that come from building Freetype's own CMakeLists
set(CMAKE_WARN_DEPRECATED OFF)
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON CACHE BOOL "" FORCE)
add_subdirectory(third_party_libs/FREETYPE)
# Restore for the rest of the project
set(CMAKE_WARN_DEPRECATED ON)

find_package(OpenGL REQUIRED)

add_library(stb_image STATIC src/internal/stb_image.cpp)
target_include_directories(stb_image PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party_libs/STB>
    $<INSTALL_INTERFACE:include>
)



# ===[UXX library]===
file(GLOB_RECURSE UXX_SRC_FILES CONFIGURE_DEPENDS "src/*.cpp")
if(NOT UXX_BUILD_GLFW_BACKEND)
    list(FILTER UXX_SRC_FILES EXCLUDE REGEX ".*UXXBackendGLFW\\.cpp$") # Drop GLFW backend source if it's disabled
endif()

add_library(UXX STATIC ${UXX_SRC_FILES})
add_library(UXX::UXX ALIAS UXX) # Namespaced alias so in-tree users can link the same way as installed ones

target_include_directories(UXX PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party_libs/GLM>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(UXX PUBLIC
    glad
    OpenGL::GL
    stb_image
    freetype
)
if(UXX_BUILD_GLFW_BACKEND)
    target_link_libraries(UXX PUBLIC glfw)
    target_compile_definitions(UXX PUBLIC UXX_HAS_GLFW_BACKEND) # Lets code #ifdef around GLFW-specific bits
endif()

if(TARGET freetype)
    target_compile_options(freetype PRIVATE -Wno-deprecated-declarations) # Freetype uses some deprecated APIs internally
endif()



# ===[Demo for testing, away from the user]===
if(UXX_BUILD_DEMO)
    add_executable(UXX_demo demoman/main.cpp)
    target_link_libraries(UXX_demo PRIVATE UXX::UXX)
endif()



# ===[Install / export so `find_package(UXX)` works downstream]===
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Register targets for export and install their built artifacts
install(TARGETS UXX glad stb_image
    EXPORT UXXTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/ DESTINATION include) # UXX's own public headers

# Write UXXTargets.cmake so downstream projects can `find_package(UXX)` and get UXX::UXX
install(EXPORT UXXTargets
    FILE UXXTargets.cmake
    NAMESPACE UXX::
    DESTINATION lib/cmake/UXX
)

# Install third-party headers too, since they're part of UXX's public include interface
install(DIRECTORY third_party_libs/GLAD/include/ DESTINATION include)
install(FILES third_party_libs/STB/stb_image.h DESTINATION include)

# Generate UXXConfig.cmake from the template, resolving install-relative paths
configure_package_config_file(
    cmake/UXXConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/UXXConfig.cmake
    INSTALL_DESTINATION lib/cmake/UXX
)
# Generate a version file so find_package(UXX 0.1) can check compatibility
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/UXXConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/UXXConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/UXXConfigVersion.cmake
    DESTINATION lib/cmake/UXX
)
