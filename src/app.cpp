#include "app.hpp"

#include <climits>
#include <cstdint>
#include <gsl/util>
#include <iostream>
#include <stdexcept>

#include <GLFW/glfw3.h>
#include <fmt/format.h>
#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>
#include <webgpu/webgpu_cpp.h>

#include "debug.hpp"

App::App(int width, int height)
    : width(width), height(height), cleanupGLFW(&glfwTerminate) {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to init GLFW");
    }
    glfwSetErrorCallback(&debug_callbacks::throwGLFW);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, false);
    window = std::shared_ptr<GLFWwindow>(
        glfwCreateWindow(width, height, "New Window", nullptr, nullptr),
        &glfwDestroyWindow);
    if (!window) {
        throw std::runtime_error("Failed to create window");
    }

    instance = createInstance();
    WGPUSurface m_surface = glfwGetWGPUSurface(instance.Get(), window.get());
    if (m_surface == nullptr) {
        throw std::runtime_error("Failed to get valid webgpu surface");
    }
    surface = wgpu::Surface::Acquire(m_surface);
    adapter = requestAdapter();
    device = requestDevice();
    device.SetUncapturedErrorCallback(&debug_callbacks::uncapturedError,
                                      nullptr);
    device.SetDeviceLostCallback(&debug_callbacks::onDeviceLost, nullptr);
    queue = device.GetQueue();
    configureSurface();
}

App::~App() {
    surface.Unconfigure();
}

void App::run() {
    while (!glfwWindowShouldClose(window.get())) {
        glfwPollEvents();
        wgpu::TextureView targetView = getNextTextureView();
        if (!targetView)
            continue;
        {
            wgpu::CommandEncoder commandEncoder = device.CreateCommandEncoder();
            {
                wgpu::RenderPassDescriptor desc{};
                wgpu::RenderPassColorAttachment attachment[1]{
                    wgpu::RenderPassColorAttachment{
                        .view = targetView,
                        .depthSlice = wgpu::kDepthSliceUndefined,
                        .resolveTarget = nullptr,
                        .loadOp = wgpu::LoadOp::Clear,
                        .storeOp = wgpu::StoreOp::Store,
                        .clearValue = wgpu::Color{0.9, 0.1, 0.2, 1.0},
                    }};
                desc.colorAttachments = attachment;
                desc.colorAttachmentCount = 1;
                desc.depthStencilAttachment = nullptr;
                desc.timestampWrites = nullptr;
                wgpu::RenderPassEncoder renderPassEncoder =
                    commandEncoder.BeginRenderPass(&desc);
                renderPassEncoder.End();
            }
            {
                wgpu::CommandBufferDescriptor desc{};
                desc.label = "Command buffer";
                wgpu::CommandBuffer buf = commandEncoder.Finish(&desc);
                queue.Submit(1, &buf);
            }
        }

        surface.Present();
        device.Tick();
    }
}

wgpu::Instance App::createInstance() {
    wgpu::InstanceDescriptor desc{};
    desc.features.timedWaitAnyEnable =
        true;  // so that we can wait on callbacks
    return wgpu::CreateInstance(&desc);
}

wgpu::Adapter App::requestAdapter() {
    wgpu::RequestAdapterOptions opts{};
    opts.forceFallbackAdapter = false;
    opts.backendType = wgpu::BackendType::Vulkan;
    opts.compatibleSurface = surface;

    wgpu::Adapter adapter;
    auto callback = [](wgpu::RequestAdapterStatus status,
                       wgpu::Adapter _adapter, const char* message,
                       wgpu::Adapter* result) {
        if (status != wgpu::RequestAdapterStatus::Success) {
            std::cerr << "Failed to obtain adapter: " << std::endl;
            return;
        }
        *result = _adapter;
    };

    instance.WaitAny(
        instance.RequestAdapter(&opts, wgpu::CallbackMode::WaitAnyOnly,
                                callback, &adapter),
        UINT64_MAX);
    if (adapter == nullptr) {
        throw std::runtime_error("Failed to create adapter. Check logs");
    }
    return adapter;
};

wgpu::Device App::requestDevice() {
    wgpu::DeviceDescriptor desc{};
    wgpu::Device device;

    auto callback = [](wgpu::RequestDeviceStatus status, wgpu::Device _device,
                       const char* message, wgpu::Device* result) {
        if (status != wgpu::RequestDeviceStatus::Success) {
            std::cerr << "Failed to obtain device: " << std::endl;
            return;
        }
        *result = _device;
    };
    auto future = adapter.RequestDevice(&desc, wgpu::CallbackMode::WaitAnyOnly,
                                        callback, &device);
    instance.WaitAny(future, UINT64_MAX);
    if (device == nullptr) {
        throw std::runtime_error("Failed to create device. Check logs");
    }
    return device;
}

void App::configureSurface() {
    wgpu::SurfaceCapabilities capabilities;
    surface.GetCapabilities(adapter, &capabilities);
    wgpu::SurfaceConfiguration config{};
    config.width = width;
    config.height = height;
    config.format = capabilities.formats[0];
    config.viewFormatCount = 0;
    config.viewFormats = nullptr;
    config.usage = wgpu::TextureUsage::RenderAttachment;
    config.device = device;
    config.presentMode = wgpu::PresentMode::Fifo;
    config.alphaMode = wgpu::CompositeAlphaMode::Auto;
    surface.Configure(&config);
}

wgpu::TextureView App::getNextTextureView() {
    wgpu::SurfaceTexture tex;
    surface.GetCurrentTexture(&tex);
    if (tex.status != wgpu::SurfaceGetCurrentTextureStatus::Success) {
        return nullptr;
    }

    wgpu::TextureViewDescriptor desc{};
    desc.label = "Surface texture view";
    desc.format = tex.texture.GetFormat();
    desc.dimension = wgpu::TextureViewDimension::e2D;
    desc.baseMipLevel = 0;
    desc.mipLevelCount = 1;
    desc.baseArrayLayer = 0;
    desc.arrayLayerCount = 1;
    desc.aspect = wgpu::TextureAspect::All;
    return tex.texture.CreateView(&desc);
}