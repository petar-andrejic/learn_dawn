#include "debug.hpp"

#include <cstddef>
#include <stdexcept>

#include <fmt/base.h>
#include <fmt/format.h>

#include "webgpu/webgpu_cpp.h"

namespace debug_callbacks {

void onDeviceLost(WGPUDeviceLostReason reason, const char* _message, void*) {
    std::string message = _message ? _message : "";
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
            fmt::println("webGPU device lost {}: {}", static_cast<int>(reason),
                         message);
            return;
        }
    }
};

void onUncapturedError(WGPUErrorType err, const char* _message, void*) {
    std::string message = _message ? _message : "";
    fmt::println("Uncaptured webGPU error {}: {}", static_cast<int>(err),
                 message);
};

void logGLFW(int err, const char* _message) {
    std::string message = _message ? _message : "";
    fmt::println("GLFW returned error {}: {}", err, message);
};

void onMapAsync(wgpu::MapAsyncStatus status, const char* _message) {
    std::string message = _message ? _message : "";
    using Status = wgpu::MapAsyncStatus;
    switch (status) {
        case Status::Success: {
            fmt::println(stderr, "Operation success: {}", message);
            return;
        };
        case Status::Aborted: {
            fmt::println(stderr, "Operation aborted: {}", message);
            return;
        }
        case Status::Error: {
            fmt::println(stderr, "Operation returned error {}: {}",
                         static_cast<int>(status), message);
            return;
        }
        case Status::InstanceDropped: {
            fmt::println("Operation aborted due to instance drop: {}", message);
            return;
        }
        default: {
            fmt::println("Operation aborted due to unkown error: {}", message);
            return;
        }
    }
}

}  // namespace debug_callbacks