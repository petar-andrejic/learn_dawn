#pragma once
#include <webgpu/webgpu.h>
#include <webgpu/webgpu_cpp.h>

namespace debug_callbacks {
void onDeviceLost(WGPUDeviceLostReason reason, const char* message, void*);

void onUncapturedError(WGPUErrorType err, const char* message, void*);

void logGLFW(int err, const char* message);

void onMapAsync(wgpu::MapAsyncStatus status, const char* message);
}  // namespace debug_callbacks