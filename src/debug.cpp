#include "debug.hpp"

#include <stdexcept>

#include <fmt/base.h>
#include <fmt/format.h>

void debug_callbacks::onDeviceLost(WGPUDeviceLostReason reason,
                                   const char* message,
                                   void*) {
    switch (reason) {
        case WGPUDeviceLostReason_Destroyed: {
            fmt::println(stderr, "webGPU device destroyed: {}", message);
            return;
        }
        case WGPUDeviceLostReason_InstanceDropped: {
            fmt::println(stderr,
                         "webGPU device lost due to instance being dropped: {}",
                         message);
            return;
        }
        default: {
            throw std::runtime_error(fmt::format("webGPU device lost {}: {}",
                                                 static_cast<int>(reason),
                                                 message));
        }
    }
};

void debug_callbacks::uncapturedError(WGPUErrorType err,
                                      const char* message,
                                      void*) {
    throw std::runtime_error(fmt::format("Uncaptured webGPU error {}: {}",
                                         static_cast<int>(err), message));
};

void debug_callbacks::throwGLFW(int err, const char* message) {
    throw std::runtime_error(
        fmt::format("GLFW return error {}: {}", err, message));
};