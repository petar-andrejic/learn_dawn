#pragma once
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

    void requestDeviceAndQueue();

    void createRenderPipeline();

    void render(const wgpu::TextureView& targetView);

    void configureSurface();

    void loadShaders();

    wgpu::TextureView getNextTextureView();
};