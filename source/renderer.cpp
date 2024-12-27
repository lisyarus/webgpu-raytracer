#include <webgpu-raytracer/renderer.hpp>
#include <webgpu-raytracer/camera_bind_group.hpp>
#include <webgpu-raytracer/compose_uniforms_bind_group.hpp>
#include <webgpu-raytracer/material_bind_group.hpp>
#include <webgpu-raytracer/geometry_bind_group.hpp>
#include <webgpu-raytracer/accumulation_bind_group.hpp>
#include <webgpu-raytracer/preview_pipeline.hpp>
#include <webgpu-raytracer/raytrace_first_hit_pipeline.hpp>
#include <webgpu-raytracer/raytrace_monte_carlo_pipeline.hpp>
#include <webgpu-raytracer/compose_pipeline.hpp>
#include <webgpu-raytracer/profiler.hpp>

struct Renderer::Impl
{
    Impl(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat surfaceFormat, ShaderRegistry & shaderRegistry);
    ~Impl();

    WGPUBindGroupLayout geometryBindGroupLayout() const { return geometryBindGroupLayout_; }
    WGPUBindGroupLayout materialBindGroupLayout() const { return materialBindGroupLayout_; }

    Mode renderMode() const { return renderMode_; }

    void setRenderMode(Mode mode);

    void resetAccumulationBuffer();

    void renderFrame(WGPUTexture surfaceTexture, Camera const & camera, SceneData const & sceneData, float exposure);

private:
    WGPUDevice device_;
    WGPUQueue queue_;
    WGPUTextureFormat surfaceFormat_;

    WGPUTexture depthTexture_ = nullptr;
    WGPUTextureView depthTextureView_ = nullptr;

    WGPUTexture accumulationTexture_ = nullptr;
    WGPUTextureView accumulationTextureView_ = nullptr;
    WGPUBindGroup accumulationStorageBindGroup_ = nullptr;
    WGPUBindGroup accumulationSampleBindGroup_ = nullptr;

    CameraBindGroup camera_;
    ComposeUniformsBindGroup composeUniforms_;

    WGPUBindGroupLayout geometryBindGroupLayout_;
    WGPUBindGroupLayout materialBindGroupLayout_;
    WGPUBindGroupLayout accumulationStorageBindGroupLayout_;
    WGPUBindGroupLayout accumulationSampleBindGroupLayout_;

    PreviewPipeline previewPipeline_;
    RaytraceFirstHitPipeline raytraceFirstHitPipeline_;
    RaytraceMonteCarloPipeline raytraceMonteCarloPipeline_;
    ComposePipeline composePipeline_;

    Mode renderMode_ = Mode::Preview;
    bool needClearAccumulationTexture_ = false;

    std::uint32_t frameID_ = 0;
    std::uint32_t globalFrameID_ = 0;

    Profiler profiler_;
};

static WGPUTextureFormat accumulationTextureFormat = WGPUTextureFormat_RGBA32Float;

Renderer::Impl::Impl(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat surfaceFormat, ShaderRegistry & shaderRegistry)
    : device_(device)
    , queue_(queue)
    , surfaceFormat_(surfaceFormat)
    , camera_(device)
    , composeUniforms_(device)
    , geometryBindGroupLayout_(createGeometryBindGroupLayout(device))
    , materialBindGroupLayout_(createMaterialBindGroupLayout(device))
    , accumulationStorageBindGroupLayout_(createAccumulationStorageBindGroupLayout(device, accumulationTextureFormat))
    , accumulationSampleBindGroupLayout_(createAccumulationSampleBindGroupLayout(device))
    , previewPipeline_(device, shaderRegistry, surfaceFormat, camera_.bindGroupLayout(), materialBindGroupLayout_)
    , raytraceFirstHitPipeline_(device, shaderRegistry, camera_.bindGroupLayout(), geometryBindGroupLayout_, materialBindGroupLayout_, accumulationStorageBindGroupLayout_)
    , raytraceMonteCarloPipeline_(device, shaderRegistry, camera_.bindGroupLayout(), geometryBindGroupLayout_, materialBindGroupLayout_, accumulationStorageBindGroupLayout_)
    , composePipeline_(device, shaderRegistry, surfaceFormat, accumulationSampleBindGroupLayout_, composeUniforms_.bindGroupLayout())
    , profiler_(device)
{}

Renderer::Impl::~Impl()
{
    profiler_.dump();

    wgpuBindGroupLayoutRelease(accumulationSampleBindGroupLayout_);
    wgpuBindGroupLayoutRelease(accumulationStorageBindGroupLayout_);
    wgpuBindGroupLayoutRelease(materialBindGroupLayout_);
    wgpuBindGroupLayoutRelease(geometryBindGroupLayout_);

    if (accumulationStorageBindGroup_)
        wgpuBindGroupRelease(accumulationStorageBindGroup_);

    if (accumulationSampleBindGroup_)
        wgpuBindGroupRelease(accumulationSampleBindGroup_);

    if (accumulationTexture_)
    {
        wgpuTextureViewRelease(accumulationTextureView_);
        wgpuTextureRelease(accumulationTexture_);
    }

    if (depthTexture_)
    {
        wgpuTextureViewRelease(depthTextureView_);
        wgpuTextureRelease(depthTexture_);
    }
}

void Renderer::Impl::setRenderMode(Mode mode)
{
    renderMode_ = mode;
    if (mode != Mode::Preview)
        resetAccumulationBuffer();
}

void Renderer::Impl::resetAccumulationBuffer()
{
    needClearAccumulationTexture_ = true;
    frameID_ = 0;
}

namespace
{

    std::pair<WGPUTexture, WGPUTextureView> recreateDepthTexture(WGPUDevice device, std::uint32_t width, std::uint32_t height)
    {
        WGPUTextureDescriptor depthTextureDescriptor;
        depthTextureDescriptor.nextInChain = nullptr;
        depthTextureDescriptor.label = "depth";
        depthTextureDescriptor.usage = WGPUTextureUsage_RenderAttachment;
        depthTextureDescriptor.dimension = WGPUTextureDimension_2D;
        depthTextureDescriptor.size = {width, height, 1};
        depthTextureDescriptor.format = WGPUTextureFormat_Depth24Plus;
        depthTextureDescriptor.mipLevelCount = 1;
        depthTextureDescriptor.sampleCount = 1;
        depthTextureDescriptor.viewFormatCount = 0;
        depthTextureDescriptor.viewFormats = nullptr;

        WGPUTexture depthTexture = wgpuDeviceCreateTexture(device, &depthTextureDescriptor);

        WGPUTextureViewDescriptor depthTextureViewDescriptor;
        depthTextureViewDescriptor.nextInChain = nullptr;
        depthTextureViewDescriptor.label = "depth";
        depthTextureViewDescriptor.format = WGPUTextureFormat_Depth24Plus;
        depthTextureViewDescriptor.dimension = WGPUTextureViewDimension_2D;
        depthTextureViewDescriptor.baseMipLevel = 0;
        depthTextureViewDescriptor.mipLevelCount = 1;
        depthTextureViewDescriptor.baseArrayLayer = 0;
        depthTextureViewDescriptor.arrayLayerCount = 1;
        depthTextureViewDescriptor.aspect = WGPUTextureAspect_DepthOnly;

        WGPUTextureView depthTextureView = wgpuTextureCreateView(depthTexture, &depthTextureViewDescriptor);

        return {depthTexture, depthTextureView};
    }

    std::pair<WGPUTexture, WGPUTextureView> recreateAccumulationTexture(WGPUDevice device, std::uint32_t width, std::uint32_t height)
    {
        WGPUTextureDescriptor accumulationTextureDescriptor;
        accumulationTextureDescriptor.nextInChain = nullptr;
        accumulationTextureDescriptor.label = "accumulation";
        accumulationTextureDescriptor.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding | WGPUTextureUsage_StorageBinding;
        accumulationTextureDescriptor.dimension = WGPUTextureDimension_2D;
        accumulationTextureDescriptor.size = {width, height, 1};
        accumulationTextureDescriptor.format = accumulationTextureFormat;
        accumulationTextureDescriptor.mipLevelCount = 1;
        accumulationTextureDescriptor.sampleCount = 1;
        accumulationTextureDescriptor.viewFormatCount = 0;
        accumulationTextureDescriptor.viewFormats = nullptr;

        WGPUTexture accumulationTexture = wgpuDeviceCreateTexture(device, &accumulationTextureDescriptor);

        WGPUTextureViewDescriptor accumulationTextureViewDescriptor;
        accumulationTextureViewDescriptor.nextInChain = nullptr;
        accumulationTextureViewDescriptor.label = "accumulation";
        accumulationTextureViewDescriptor.format = accumulationTextureFormat;
        accumulationTextureViewDescriptor.dimension = WGPUTextureViewDimension_2D;
        accumulationTextureViewDescriptor.baseMipLevel = 0;
        accumulationTextureViewDescriptor.mipLevelCount = 1;
        accumulationTextureViewDescriptor.baseArrayLayer = 0;
        accumulationTextureViewDescriptor.arrayLayerCount = 1;
        accumulationTextureViewDescriptor.aspect = WGPUTextureAspect_All;

        WGPUTextureView accumulationTextureView = wgpuTextureCreateView(accumulationTexture, &accumulationTextureViewDescriptor);

        return {accumulationTexture, accumulationTextureView};
    }

}

void Renderer::Impl::renderFrame(WGPUTexture surfaceTexture, Camera const & camera, SceneData const & sceneData, float exposure)
{
    glm::uvec2 const screenSize{wgpuTextureGetWidth(surfaceTexture), wgpuTextureGetHeight(surfaceTexture)};

    camera_.update(queue_, camera, screenSize, frameID_, globalFrameID_);
    composeUniforms_.update(queue_, exposure);

    WGPUCommandEncoderDescriptor commandEncoderDescriptor;
    commandEncoderDescriptor.nextInChain = nullptr;
    commandEncoderDescriptor.label = nullptr;

    WGPUCommandEncoder commandEncoder = wgpuDeviceCreateCommandEncoder(device_, &commandEncoderDescriptor);

    auto frameProfiler = profiler_.beginFrame(commandEncoder);

    WGPUTextureViewDescriptor surfaceTextureViewDescriptor;
    surfaceTextureViewDescriptor.nextInChain = nullptr;
    surfaceTextureViewDescriptor.label = nullptr;
    surfaceTextureViewDescriptor.format = surfaceFormat_;
    surfaceTextureViewDescriptor.dimension = WGPUTextureViewDimension_2D;
    surfaceTextureViewDescriptor.baseMipLevel = 0;
    surfaceTextureViewDescriptor.mipLevelCount = 1;
    surfaceTextureViewDescriptor.baseArrayLayer = 0;
    surfaceTextureViewDescriptor.arrayLayerCount = 1;
    surfaceTextureViewDescriptor.aspect = WGPUTextureAspect_All;

    WGPUTextureView surfaceTextureView = wgpuTextureCreateView(surfaceTexture, &surfaceTextureViewDescriptor);

    if (renderMode_ == Mode::Preview)
    {
        if (!depthTexture_ || wgpuTextureGetWidth(depthTexture_) != screenSize.x || wgpuTextureGetHeight(depthTexture_) != screenSize.y)
        {
            if (depthTexture_)
            {
                wgpuTextureViewRelease(depthTextureView_);
                wgpuTextureRelease(depthTexture_);
            }

            std::tie(depthTexture_, depthTextureView_) = recreateDepthTexture(device_, screenSize.x, screenSize.y);
        }

        renderPreview(previewPipeline_, commandEncoder, surfaceTextureView, depthTextureView_, camera_.bindGroup(), sceneData);
    }
    else
    {
        if (!accumulationTexture_ || wgpuTextureGetWidth(accumulationTexture_) != screenSize.x || wgpuTextureGetHeight(accumulationTexture_) != screenSize.y)
        {
            if (accumulationTexture_)
            {
                wgpuTextureViewRelease(accumulationTextureView_);
                wgpuTextureRelease(accumulationTexture_);
            }

            std::tie(accumulationTexture_, accumulationTextureView_) = recreateAccumulationTexture(device_, screenSize.x, screenSize.y);

            if (accumulationStorageBindGroup_)
                wgpuBindGroupRelease(accumulationStorageBindGroup_);

            if (accumulationSampleBindGroup_)
                wgpuBindGroupRelease(accumulationSampleBindGroup_);

            accumulationSampleBindGroup_ = createAccumulationSampleBindGroup(device_, accumulationSampleBindGroupLayout_, accumulationTextureView_);
            accumulationStorageBindGroup_ = createAccumulationStorageBindGroup(device_, accumulationStorageBindGroupLayout_, accumulationTextureView_);
        }

        if (renderMode_ == Mode::RaytraceFirstHit)
            renderRaytraceFirstHit(commandEncoder, accumulationTextureView_, raytraceFirstHitPipeline_.pipeline(),
                camera_.bindGroup(), sceneData, accumulationStorageBindGroup_, screenSize);
        else if (renderMode_ == Mode::RaytraceMonteCarlo)
        {
            renderRaytraceMonteCarlo(commandEncoder, accumulationTextureView_, raytraceMonteCarloPipeline_.pipeline(),
                camera_.bindGroup(), sceneData, accumulationStorageBindGroup_, screenSize);
            frameProfiler.timestamp("raytrace");
        }

        renderCompose(commandEncoder, surfaceTextureView, composePipeline_.renderPipeline(), accumulationSampleBindGroup_, composeUniforms_.bindGroup());
        frameProfiler.timestamp("compose");

        needClearAccumulationTexture_ = false;
    }

    profiler_.endFrame(std::move(frameProfiler));

    WGPUCommandBufferDescriptor commandBufferDescriptor;
    commandBufferDescriptor.nextInChain = nullptr;
    commandBufferDescriptor.label = nullptr;

    WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(commandEncoder, &commandBufferDescriptor);

    wgpuQueueSubmit(queue_, 1, &commandBuffer);

    wgpuCommandBufferRelease(commandBuffer);
    wgpuTextureViewRelease(surfaceTextureView);
    wgpuCommandEncoderRelease(commandEncoder);

    ++frameID_;
    ++globalFrameID_;

    profiler_.poll();
}

Renderer::Renderer(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat surfaceFormat, ShaderRegistry & shaderRegistry)
    : pimpl_(std::make_unique<Impl>(device, queue, surfaceFormat, shaderRegistry))
{}

// Need to explicitly implement the destructor for pimpl to work
// See https://www.fluentcpp.com/2017/09/22/make-pimpl-using-unique_ptr
Renderer::~Renderer() = default;

WGPUBindGroupLayout Renderer::geometryBindGroupLayout() const
{
    return pimpl_->geometryBindGroupLayout();
}

WGPUBindGroupLayout Renderer::materialBindGroupLayout() const
{
    return pimpl_->materialBindGroupLayout();
}

Renderer::Mode Renderer::renderMode() const
{
    return pimpl_->renderMode();
}

void Renderer::setRenderMode(Mode mode)
{
    pimpl_->setRenderMode(mode);
}

void Renderer::renderFrame(WGPUTexture surfaceTexture, Camera const & camera, SceneData const & sceneData, float exposure)
{
    pimpl_->renderFrame(surfaceTexture, camera, sceneData, exposure);
}
