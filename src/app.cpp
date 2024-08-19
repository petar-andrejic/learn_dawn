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
#include <numeric>
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

void App::createWindow(const wgpu::Extent2D& dims) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = std::shared_ptr<GLFWwindow>(
        glfwCreateWindow(static_cast<int>(dims.width),
                         static_cast<int>(dims.height), "New Window", nullptr,
                         nullptr),
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
    createWindow(dimensions);
}

App::App(const wgpu::Extent2D& dims)
    : onDestroy(&glfwTerminate), dimensions(dims) {
    data.load("./resources/data.txt");
    initGLFW();
    initWebGPU();
    configureSurface();
    loadShaders();
    createRenderPipeline();
}

App::~App() noexcept {
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
                    .clearValue = wgpu::Color{0.5, 0.5, 0.5, 1.0},
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
            renderPassEncoder.SetVertexBuffer(0, vertexBuffer);
            renderPassEncoder.SetIndexBuffer(indexBuffer,
                                             wgpu::IndexFormat::Uint16);
            renderPassEncoder.DrawIndexed(data.index.size(), 1, 0, 0, 0);
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
    while (!glfwWindowShouldClose(window.get())) {
        glfwPollEvents();
        wgpu::TextureView targetView = getNextTextureView();
        if (!targetView)
            continue;
        render(targetView);
    }
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

wgpu::RequiredLimits App::getRequiredLimits() {
    // wgpu::SupportedLimits supportedLimits;
    // adapter.GetLimits(&supportedLimits);
    wgpu::RequiredLimits requiredLimits{
        .limits{
            .maxVertexBuffers = 1,
            .maxBufferSize = data.maxBufferSize(),
            .maxVertexAttributes = 2,
            .maxVertexBufferArrayStride = 5 * sizeof(float),
            .maxInterStageShaderComponents = 3,
        },
    };
    return requiredLimits;
}

void App::requestDeviceAndQueue() {
    wgpu::RequiredLimits limits = getRequiredLimits();
    wgpu::DeviceDescriptor desc({
        .requiredLimits = &limits,
    });

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
        .width = dimensions.width,
        .height = dimensions.height,
        .presentMode = wgpu::PresentMode::Fifo,
    };
    surface.Configure(&config);
}

void App::loadShaders() {
    std::string tmp, shaderSource;
    std::ifstream file("./resources/shader.wgsl");
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
    wgpu::BufferDescriptor vertex_desc{
        .label = "Vertex Buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
        .size = align4(data.vertex.size() * sizeof(float)),
        .mappedAtCreation = false,
    };

    vertexBuffer = device.CreateBuffer(&vertex_desc);
    queue.WriteBuffer(vertexBuffer, 0, data.vertex.data(), vertex_desc.size);

    wgpu::BufferDescriptor index_desc{
        .label = "Index Buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index,
        .size = align4(data.index.size() * sizeof(uint16_t)),
        .mappedAtCreation = false,
    };

    indexBuffer = device.CreateBuffer(&index_desc);
    queue.WriteBuffer(indexBuffer, 0, data.index.data(), index_desc.size);
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
    std::array<wgpu::VertexAttribute, 2> attribs{
        wgpu::VertexAttribute{
            .format = wgpu::VertexFormat::Float32x2,
            .offset = 0,
            .shaderLocation = 0,
        },  // position
        wgpu::VertexAttribute{
            .format = wgpu::VertexFormat::Float32x3,
            .offset = 2 * sizeof(float),
            .shaderLocation = 1,
        }};  // colour
    wgpu::VertexBufferLayout vbl{
        .arrayStride = 5 * sizeof(float),
        .stepMode = wgpu::VertexStepMode::Vertex,
        .attributeCount = attribs.size(),
        .attributes = attribs.data(),
    };
    wgpu::VertexState vs{
        .module = shaderModule,
        .entryPoint = "vs_main",
        .constantCount = 0,
        .constants = nullptr,
        .bufferCount = 1,
        .buffers = &vbl,
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