cmake_minimum_required(VERSION 3.13)

project(learn_dawn
        VERSION 0.1.0
        LANGUAGES CXX C
)
set(dawn_DAWN_FETCH_DEPENDENCIES CACHE STRING ON)
add_subdirectory(dawn)



include(cmake/CPM.cmake)

CPMAddPackage("gh:Microsoft/GSL@4.0.0")
CPMAddPackage("gh:fmtlib/fmt#11.0.2")

add_executable(App)
add_custom_command(
        TARGET App POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/shaders"
        "${PROJECT_BINARY_DIR}/shaders"
)
set_target_properties(App PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        COMPILE_WARNING_AS_ERROR ON
)

get_target_property(webgpu_target dawn::webgpu_dawn ALIASED_TARGET)
add_library(webgpu ALIAS ${webgpu_target}) # required alias for below
add_subdirectory(glfw3webgpu) # until https://github.com/glfw/glfw/pull/2333 is merged

target_link_libraries(App PRIVATE dawn::webgpu_dawn glfw Microsoft.GSL::GSL fmt::fmt glfw3webgpu)
add_subdirectory(src)

