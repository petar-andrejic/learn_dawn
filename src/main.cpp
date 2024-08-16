#include <cstdlib>

#include <webgpu/webgpu_cpp.h>

#include "app.hpp"

int main(int argc, char* argv[]) {
    App app = App(800, 600);
    app.run();
}
