#include <cstdlib>

#include <fmt/format.h>
#include <webgpu/webgpu_cpp.h>

#include "app.hpp"

auto main(int /*argc*/, char* /*argv*/[]) -> int {
    try {
        App app({800, 600});
        app.run();
    } catch (std::runtime_error e) {
        fmt::println(stderr, "Program terminated with runtime error: {}",
                     e.what());
    } catch (std::exception e) {
        fmt::println(stderr, "Program terminated with exception: {}", e.what());
    } catch (...) {
        fmt::println(stderr, "Program terminated with unkown exception");
    };
}
