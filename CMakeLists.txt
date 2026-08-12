# ===[CMAKE setup]===
cmake_minimum_required(VERSION 3.31.11)
project(UXX VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(UXX_BUILD_DEMO "Build the UXX demo executable" ON)
option(UXX_BUILD_GLFW_BACKEND "Build the GLFW window backend" ON)

add_compile_definitions(GLM_ENABLE_EXPERIMENTAL)



# ===[Third-party deps]===
add_library(glad STATIC third_party_libs/GLAD/src/glad.c)
target_include_directories(glad PUBLIC third_party_libs/GLAD/include)

if(UXX_BUILD_GLFW_BACKEND)
    add_subdirectory(third_party_libs/GLFW)
endif()

set(CMAKE_WARN_DEPRECATED OFF)
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON CACHE BOOL "" FORCE)
add_subdirectory(third_party_libs/FREETYPE)
set(CMAKE_WARN_DEPRECATED ON)

find_package(OpenGL REQUIRED)

add_library(stb_image STATIC src/internal/stb_image.cpp)
target_include_directories(stb_image PUBLIC third_party_libs/STB)



# ===[UXX library]===
file(GLOB_RECURSE UXX_SRC_FILES CONFIGURE_DEPENDS "src/*.cpp")
if(NOT UXX_BUILD_GLFW_BACKEND)
    list(FILTER UXX_SRC_FILES EXCLUDE REGEX ".*GLFWBackend\\.cpp$")
endif()

add_library(UXX STATIC ${UXX_SRC_FILES})
add_library(UXX::UXX ALIAS UXX)

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
    target_compile_definitions(UXX PUBLIC UXX_HAS_GLFW_BACKEND)
endif()

if(TARGET freetype)
    target_compile_options(freetype PRIVATE -Wno-deprecated-declarations)
endif()



# ===[Demo for testing, away from the user]===
if(UXX_BUILD_DEMO)
    add_executable(UXX_demo demoman/main.cpp)
    target_link_libraries(UXX_demo PRIVATE UXX::UXX)
endif()



# ===[Install / export so `find_package(UXX)` works downstream]===
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS UXX glad stb_image
    EXPORT UXXTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/ DESTINATION include)

install(EXPORT UXXTargets
    FILE UXXTargets.cmake
    NAMESPACE UXX::
    DESTINATION lib/cmake/UXX
)

configure_package_config_file(
    cmake/UXXConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/UXXConfig.cmake
    INSTALL_DESTINATION lib/cmake/UXX
)
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
