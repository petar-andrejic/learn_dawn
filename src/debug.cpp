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
            throw std::runtime_error(fmt::format("webGPU device lost {}: {}",
                                                 static_cast<int>(reason),
                                                 message));
        }
    }
};

void uncapturedError(WGPUErrorType err, const char* _message, void*) {
    std::string message = _message ? _message : "";
    throw std::runtime_error(fmt::format("Uncaptured webGPU error {}: {}",
                                         static_cast<int>(err), message));
};

void throwGLFW(int err, const char* _message) {
    std::string message = _message ? _message : "";

    throw std::runtime_error(
        fmt::format("GLFW return error {}: {}", err, message));
};

void mapAsyncStatus(wgpu::MapAsyncStatus status, const char* _message) {
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
            const std::string msg =
                fmt::format("Operation returned error {}: {}",
                            static_cast<size_t>(status), message);
            throw std::runtime_error(msg);
        }
        case Status::InstanceDropped: {
            const std::string msg = fmt::format(
                "Operation aborted due to instance drop: {}", message);
            throw std::runtime_error(msg);
        }
        default: {
            const std::string msg = fmt::format(
                "Operation aborted due to unkown error: {}", message);
            throw std::runtime_error(msg);
        }
    }
}

}  // namespace debug_callbacks