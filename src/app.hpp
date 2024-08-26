#pragma once
#include <cstddef>
#include <gsl/util>
#include <memory>

#include <GLFW/glfw3.h>
#include <webgpu/webgpu_cpp.h>

#include "loader.hpp"

constexpr auto align4(const size_t& size) -> size_t {
    return (size + 3U) & ~3U;
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
    wgpu::BindGroup bindGroup;
    wgpu::RenderPipeline pipeline;

    Data data;

    // buffers
    wgpu::Buffer vertexBuffer, indexBuffer, uniformBuffer;

    wgpu::Extent2D dimensions;

    void createSurface();
    void createWindow(const wgpu::Extent2D& dims);
    void initWebGPU();
    void initGLFW();
    App(const wgpu::Extent2D& dims);
    ~App() noexcept;

    App(const App& other) = delete;
    App(App&& other) noexcept = default;

    auto operator=(App&& other) noexcept -> App& {
        using std::swap;
        swap(*this, other);
        return *this;
    }

    auto operator=(const App& other) -> App& = delete;

    void run();

   private:
    void createInstance();

    void requestAdapter();

    auto getRequiredLimits() -> wgpu::RequiredLimits;

    void requestDeviceAndQueue();

    void createRenderPipeline();

    void initBuffers();

    void render(const wgpu::TextureView& targetView);

    void configureSurface();

    void loadShaders();

    auto getNextTextureView() -> wgpu::TextureView;
};