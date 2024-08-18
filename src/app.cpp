// ReSharper disable CppMemberFunctionMayBeConst
// Promising const here is a bit misleading, since the wgpu classes obfuscate
// their state due to the C api, and as such const mainly amounts to promising
// their pointers don't change, which isn't always desirable
#include "app.hpp"

#include <algorithm>
#include <climits>
#include <cstdint>
#include <fstream>
#include <gsl/util>
#include <iostream>
#include <ranges>
#include <stdexcept>
#include <vector>

#include <GLFW/glfw3.h>
#include <fmt/base.h>
#include <fmt/format.h>
#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>
#include <webgpu/webgpu_cpp.h>

#include "debug.hpp"

void App::createSurface() {
    WGPUSurface m_surface = glfwGetWGPUSurface(instance.Get(), window.get());
    if (!m_surface) {
        throw std::runtime_error("Failed to get valid webGPU surface");
    }
    surface = wgpu::Surface::Acquire(m_surface);
}

void App::createWindow(const int width, const int height) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, false);
    window = std::shared_ptr<GLFWwindow>(
        glfwCreateWindow(width, height, "New Window", nullptr, nullptr),
        &glfwDestroyWindow);
    if (!window) {
        throw std::runtime_error("Failed to create window");
    }
}

void App::initWebGPU() {
    createInstance();
    createSurface();
    requestAdapter();
    requestDeviceAndQueue();
    initBuffers();
}

void App::initGLFW() {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to init GLFW");
    }
    glfwSetErrorCallback(&debug_callbacks::throwGLFW);
    createWindow(static_cast<int>(width), static_cast<int>(height));
}

App::App(const int width, const int height)
    : cleanupGLFW(&glfwTerminate), width(width), height(height) {
    initGLFW();
    initWebGPU();
    configureSurface();
    loadShaders();
    createRenderPipeline();
}

App::~App() {
    surface.Unconfigure();
}

void App::render(const wgpu::TextureView& targetView) {
    {
        wgpu::CommandEncoder commandEncoder = device.CreateCommandEncoder();
        {
            wgpu::RenderPassColorAttachment attachment[1]{
                wgpu::RenderPassColorAttachment{
                    .view = targetView,
                    .depthSlice = wgpu::kDepthSliceUndefined,
                    .resolveTarget = nullptr,
                    .loadOp = wgpu::LoadOp::Clear,
                    .storeOp = wgpu::StoreOp::Store,
                    .clearValue = wgpu::Color{0.9, 0.1, 0.2, 1.0},
                },
            };
            wgpu::RenderPassDescriptor desc{
                .colorAttachmentCount = 1,
                .colorAttachments = attachment,
                .depthStencilAttachment = nullptr,
                .timestampWrites = nullptr,
            };
            wgpu::RenderPassEncoder renderPassEncoder =
                commandEncoder.BeginRenderPass(&desc);
            renderPassEncoder.SetPipeline(pipeline);
            renderPassEncoder.Draw(3, 1, 0, 0);
            renderPassEncoder.End();
        }
        {
            wgpu::CommandBufferDescriptor desc{
                .label = "Command buffer",
            };
            const wgpu::CommandBuffer buf = commandEncoder.Finish(&desc);
            queue.Submit(1, &buf);
        }
    }

    surface.Present();
    device.Tick();
}

void App::run() {
    fillAndCopyBuffers();
    while (!glfwWindowShouldClose(window.get())) {
        glfwPollEvents();
        wgpu::TextureView targetView = getNextTextureView();
        if (!targetView)
            continue;
        render(targetView);
    }
}

void App::fillAndCopyBuffers() {
    using namespace std::ranges;
    auto numbers = views::iota(0u, 16u) | to<std::vector<uint8_t>>();
    assert(numbers.size() == 16u);
    queue.WriteBuffer(buffer_1, 0, numbers.data(), numbers.size());

    wgpu::CommandEncoder commandEncoder = device.CreateCommandEncoder();
    commandEncoder.CopyBufferToBuffer(buffer_1, 0, buffer_2, 0, 16);
    wgpu::CommandBuffer command = commandEncoder.Finish();
    queue.Submit(1, &command);

    wgpu::Future future = buffer_2.MapAsync(wgpu::MapMode::Read, 0, 16,
                                            wgpu::CallbackMode::WaitAnyOnly,
                                            &debug_callbacks::mapAsyncStatus);
    instance.WaitAny(future, UINT64_MAX);

    const uint8_t* result_ptr =
        static_cast<const uint8_t*>(buffer_2.GetConstMappedRange(0, 16));
    for_each(std::span(result_ptr, 16) | views::enumerate,
             [](auto tup) -> void {
                 long idx;
                 uint8_t val;
                 std::tie(idx, val) = tup;
                 fmt::println("Result[{}] = {}", idx, val);
             });
}

// ReSharper disable once CppMemberFunctionMayBeStatic
// There's no point doing this outside of the constructor
void App::createInstance() {  // NOLINT(*-convert-member-functions-to-static)
    wgpu::InstanceDescriptor desc{
        .features{
            .timedWaitAnyEnable = true,
        },
    };
    instance = wgpu::CreateInstance(&desc);
    if (!instance) {
        throw std::runtime_error("Failed to create webGPU instance.");
    }
}

void App::requestAdapter() {
    wgpu::RequestAdapterOptions opts{
        .compatibleSurface = surface,
        .forceFallbackAdapter = false,
    };

    // ReSharper disable once CppParameterMayBeConst
    // ReSharper disable once CppPassValueParameterByConstReference
    // The signature needs to match that requested by wgpu
    auto callback = [](wgpu::RequestAdapterStatus status,
                       wgpu::Adapter _adapter, const char* message,
                       wgpu::Adapter* result) {
        if (status != wgpu::RequestAdapterStatus::Success) {
            std::cerr << "Failed to obtain adapter: " << std::endl;
            return;
        }
        *result = std::move(_adapter);
    };

    wgpu::Future future = instance.RequestAdapter(
        &opts, wgpu::CallbackMode::WaitAnyOnly, callback, &adapter);
    instance.WaitAny(future, UINT64_MAX);
    if (!adapter) {
        throw std::runtime_error("Failed to create adapter. Check logs");
    }
};

void App::requestDeviceAndQueue() {
    wgpu::DeviceDescriptor desc{};

    // ReSharper disable once CppParameterMayBeConst
    // ReSharper disable once CppPassValueParameterByConstReference
    // The signature needs to match that requested by wgpu
    auto callback = [](wgpu::RequestDeviceStatus status, wgpu::Device _device,
                       const char* message, wgpu::Device* result) {
        if (status != wgpu::RequestDeviceStatus::Success) {
            std::cerr << "Failed to obtain device: " << std::endl;
            return;
        }
        *result = std::move(_device);
    };

    wgpu::Future future = adapter.RequestDevice(
        &desc, wgpu::CallbackMode::WaitAnyOnly, callback, &device);
    instance.WaitAny(future, UINT64_MAX);
    if (!device) {
        throw std::runtime_error("Failed to create device. Check logs");
    }
    device.SetUncapturedErrorCallback(&debug_callbacks::uncapturedError,
                                      nullptr);
    device.SetDeviceLostCallback(&debug_callbacks::onDeviceLost, nullptr);
    queue = device.GetQueue();
    if (!queue) {
        throw std::runtime_error("Failed to create queue. Check logs");
    };
}

void App::configureSurface() {
    wgpu::SurfaceCapabilities capabilities;
    surface.GetCapabilities(adapter, &capabilities);
    surfaceFormat = capabilities.formats[0];
    wgpu::SurfaceConfiguration config{
        .device = device,
        .format = surfaceFormat,
        .usage = wgpu::TextureUsage::RenderAttachment,
        .viewFormatCount = 0,
        .viewFormats = nullptr,
        .alphaMode = wgpu::CompositeAlphaMode::Auto,
        .width = width,
        .height = height,
        .presentMode = wgpu::PresentMode::Fifo,
    };
    surface.Configure(&config);
}

void App::loadShaders() {
    std::string tmp, shaderSource;
    std::ifstream file("./shaders/shader.wgsl");
    while (std::getline(file, tmp)) {
        shaderSource += tmp + "\n";
    }
    wgpu::ShaderModuleWGSLDescriptor wgsl_desc({
        .code = shaderSource.c_str(),
    });
    wgpu::ShaderModuleDescriptor sm_desc{
        .nextInChain = &wgsl_desc,
    };

    shaderModule = device.CreateShaderModule(&sm_desc);
}

void App::initBuffers() {
    wgpu::BufferDescriptor desc_1{
        .label = "Input Buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc,
        .size = 16,
        .mappedAtCreation = false,
    };

    wgpu::BufferDescriptor desc_2{
        .label = "Output Buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead,
        .size = 16,
        .mappedAtCreation = false,
    };

    buffer_1 = device.CreateBuffer(&desc_1);
    buffer_2 = device.CreateBuffer(&desc_2);
}

wgpu::TextureView App::getNextTextureView() {
    wgpu::SurfaceTexture tex;
    surface.GetCurrentTexture(&tex);
    if (tex.status != wgpu::SurfaceGetCurrentTextureStatus::Success) {
        return nullptr;
    }

    wgpu::TextureViewDescriptor desc{
        .label = "Surface texture view",
        .format = tex.texture.GetFormat(),
        .dimension = wgpu::TextureViewDimension::e2D,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1,
        .aspect = wgpu::TextureAspect::All,
    };
    return tex.texture.CreateView(&desc);
}

void App::createRenderPipeline() {
    wgpu::VertexState vs{
        .module = shaderModule,
        .entryPoint = "vs_main",
        .constantCount = 0,
        .constants = nullptr,
        .bufferCount = 0,
        .buffers = nullptr,
    };

    wgpu::PrimitiveState ps{
        .topology = wgpu::PrimitiveTopology::TriangleList,
        .stripIndexFormat = wgpu::IndexFormat::Undefined,
        .frontFace = wgpu::FrontFace::CCW,
        .cullMode = wgpu::CullMode::None,
    };

    wgpu::BlendState bs{
        .color{
            .operation = wgpu::BlendOperation::Add,
            .srcFactor = wgpu::BlendFactor::SrcAlpha,
            .dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha,
        },
        .alpha{
            .operation = wgpu::BlendOperation::Add,
            .srcFactor = wgpu::BlendFactor::Zero,
            .dstFactor = wgpu::BlendFactor::One,
        },
    };

    wgpu::ColorTargetState cts{
        .format = surfaceFormat,
        .blend = &bs,
        .writeMask = wgpu::ColorWriteMask::All,
    };

    wgpu::FragmentState fs{
        .module = shaderModule,
        .entryPoint = "fs_main",
        .constantCount = 0,
        .constants = nullptr,
        .targetCount = 1,
        .targets = &cts,
    };

    wgpu::RenderPipelineDescriptor desc{
        .vertex = vs,
        .primitive = ps,
        .depthStencil = nullptr,
        .multisample{
            .count = 1,
            .mask = ~0u,
            .alphaToCoverageEnabled = false,
        },
        .fragment = &fs,
    };
    pipeline = device.CreateRenderPipeline(&desc);
}