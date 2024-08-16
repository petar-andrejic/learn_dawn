#pragma once
#include <gsl/util>
#include <memory>

#include <GLFW/glfw3.h>
#include <webgpu/webgpu_cpp.h>

struct wgpuData {};

struct App {
   private:
    gsl::final_action<void (*)()> cleanupGLFW;

   public:
    std::shared_ptr<GLFWwindow> window;
    wgpu::Instance instance;
    wgpu::Adapter adapter;
    wgpu::Surface surface;
    wgpu::Device device;
    wgpu::Queue queue;

    int width, height;

    App(int width, int height);

    ~App();

    void run();

   private:
    wgpu::Instance createInstance();

    wgpu::Adapter requestAdapter();

    wgpu::Device requestDevice();

    void configureSurface();

    wgpu::TextureView getNextTextureView();
};