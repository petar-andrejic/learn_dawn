#pragma once
#include <cstddef>
#include <gsl/util>
#include <memory>

#include <GLFW/glfw3.h>
#include <webgpu/webgpu_cpp.h>

struct App {
   private:
    gsl::final_action<void (*)()> cleanupGLFW;

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

    constexpr static size_t vertexCount = 6;
    constexpr static size_t vertexStride = 5;
    constexpr static std::array<float, vertexCount * vertexStride> vertexData =
        {
            // x0,  y0,  r0,  g0,  b0
            -0.5, -0.5, 1.0, 0.0, 0.0,

            // x1,  y1,  r1,  g1,  b1
            +0.5, -0.5, 0.0, 1.0, 0.0,

            // ...
            +0.0, +0.5, 0.0, 0.0, 1.0,
            //
            -0.55f, -0.5, 1.0, 1.0, 0.0,
            //
            -0.05f, +0.5, 1.0, 0.0, 1.0,
            //
            -0.55f, +0.5, 0.0, 1.0, 1.0};
    // buffers
    wgpu::Buffer buffer_1;
    wgpu::Buffer buffer_2;
    wgpu::Buffer vertexBuffer;

    unsigned int width, height;

    void createSurface();
    void createWindow(int width, int height);
    void initWebGPU();
    void initGLFW();
    App(int width, int height);

    ~App();

    void run();

   private:
    void createInstance();

    void requestAdapter();

    wgpu::RequiredLimits getRequiredLimits();

    void requestDeviceAndQueue();

    void createRenderPipeline();

    void initBuffers();

    void render(const wgpu::TextureView& targetView);

    void fillAndCopyBuffers();

    void configureSurface();

    void loadShaders();

    wgpu::TextureView getNextTextureView();
};