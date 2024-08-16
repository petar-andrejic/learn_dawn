cmake_minimum_required(VERSION 3.13)

project(hello_webgpu
    VERSION 0.1.0
    LANGUAGES CXX C
)
add_subdirectory(dawn)
include(cmake/CPM.cmake)

CPMAddPackage("gh:Microsoft/GSL@4.0.0")
CPMAddPackage("gh:fmtlib/fmt#11.0.2")
# CPMAddPackage("gh:glfw/glfw#3.4")
# CPMAddPackage("gh:google/dawn#main")

# find_package(Dawn REQUIRED)
# find_package(glfw3 REQUIRED)

add_executable(App)

set_target_properties(App PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    COMPILE_WARNING_AS_ERROR ON
)
add_subdirectory(glfw3webgpu)

target_link_libraries(App PRIVATE dawn::webgpu_dawn glfw Microsoft.GSL::GSL fmt::fmt glfw3webgpu)
# target_compile_definitions(App PRIVATE DAWN_USE_WAYLAND)
add_subdirectory(src)

