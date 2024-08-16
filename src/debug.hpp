#pragma once
#include <webgpu/webgpu.h>

namespace debug_callbacks {
void onDeviceLost(WGPUDeviceLostReason reason, const char* message, void*);

void uncapturedError(WGPUErrorType err, const char* message, void*);

void throwGLFW(int err, const char* message);

}  // namespace debug_callbacks