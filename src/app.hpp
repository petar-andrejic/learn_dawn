#pragma once
#include <cstddef>
#include <gsl/util>
#include <memory>

#include <GLFW/glfw3.h>
#include <webgpu/webgpu_cpp.h>

#include "aligned_alloc.hpp"

inline constexpr size_t align4(const size_t& sz) {
    return (sz + 3) & ~3;
};

struct App {
   private:
    gsl::final_action<void (*)()> onDestroy;

   public:
    std::shared_ptr<GLFWwindow> window;
    wgpu::Instance instance;
    wgpu::Adapter adapter;
    wgpu::Surface surface;
    wgpu::TextureFormat surfaceFormat = wgpu::TextureFormat::Undefined;
    wgpu::Device device;
    wgpu::Queue queue;
    wgpu::ShaderModule shaderModule;
    wgpu::RenderPipeline pipeline;

    const static alignedVector<float> vertexData;
    const static alignedVector<uint16_t> indexData;

    // buffers
    wgpu::Buffer vertexBuffer, indexBuffer;

    unsigned int width, height;

    void createSurface();
    void createWindow(int width, int height);
    void initWebGPU();
    void initGLFW();
    App(int width, int height);

    ~App() noexcept;

    void run();

   private:
    void createInstance();

    void requestAdapter();

    wgpu::RequiredLimits getRequiredLimits();

    void requestDeviceAndQueue();

    void createRenderPipeline();

    void initBuffers();

    void render(const wgpu::TextureView& targetView);

    void configureSurface();

    void loadShaders();

    wgpu::TextureView getNextTextureView();
};