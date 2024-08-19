#include <cstdlib>

#include <webgpu/webgpu_cpp.h>

#include "app.hpp"

auto main(int /*argc*/, char* /*argv*/[]) -> int {
    App app({800, 600});
    app.run();
}
