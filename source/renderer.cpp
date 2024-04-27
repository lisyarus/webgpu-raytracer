#include <webgpu-raytracer/renderer.hpp>
#include <webgpu-raytracer/camera_bind_group.hpp>
#include <webgpu-raytracer/preview_pipeline.hpp>

struct Renderer::Impl
{
    Impl(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat surfaceFormat, ShaderRegistry & shaderRegistry);
    ~Impl();

    void renderFrame(WGPUTexture surfaceTexture, Camera const & camera, SceneData const & sceneData);

private:
    WGPUDevice device_;
    WGPUQueue queue_;
    WGPUTextureFormat surfaceFormat_;

    WGPUTexture depthTexture_ = nullptr;
    WGPUTextureView depthTextureView_ = nullptr;

    CameraBindGroup camera_;

    PreviewPipeline previewPipeline_;
};

Renderer::Impl::Impl(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat surfaceFormat, ShaderRegistry & shaderRegistry)
    : device_(device)
    , queue_(queue)
    , surfaceFormat_(surfaceFormat)
    , camera_(device)
    , previewPipeline_(device, shaderRegistry, surfaceFormat, camera_.bindGroupLayout())
{}

Renderer::Impl::~Impl()
{
    if (depthTexture_)
    {
        wgpuTextureViewRelease(depthTextureView_);
        wgpuTextureRelease(depthTexture_);
    }
}

void Renderer::Impl::renderFrame(WGPUTexture surfaceTexture, Camera const & camera, SceneData const & sceneData)
{
    std::uint32_t surfaceWidth = wgpuTextureGetWidth(surfaceTexture);
    std::uint32_t surfaceHeight = wgpuTextureGetHeight(surfaceTexture);

    if (!depthTexture_ || wgpuTextureGetWidth(depthTexture_) != surfaceWidth || wgpuTextureGetHeight(depthTexture_) != surfaceHeight)
    {
        if (depthTexture_)
        {
            wgpuTextureViewRelease(depthTextureView_);
            wgpuTextureRelease(depthTexture_);
        }

        WGPUTextureDescriptor depthTextureDescriptor;
        depthTextureDescriptor.nextInChain = nullptr;
        depthTextureDescriptor.label = "depth";
        depthTextureDescriptor.usage = WGPUTextureUsage_RenderAttachment;
        depthTextureDescriptor.dimension = WGPUTextureDimension_2D;
        depthTextureDescriptor.size = {surfaceWidth, surfaceHeight, 1};
        depthTextureDescriptor.format = WGPUTextureFormat_Depth24Plus;
        depthTextureDescriptor.mipLevelCount = 1;
        depthTextureDescriptor.sampleCount = 1;
        depthTextureDescriptor.viewFormatCount = 0;
        depthTextureDescriptor.viewFormats = nullptr;

        depthTexture_ = wgpuDeviceCreateTexture(device_, &depthTextureDescriptor);

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

        depthTextureView_ = wgpuTextureCreateView(depthTexture_, &depthTextureViewDescriptor);
    }

    camera_.update(queue_, camera);

    WGPUCommandEncoderDescriptor commandEncoderDescriptor;
    commandEncoderDescriptor.nextInChain = nullptr;
    commandEncoderDescriptor.label = nullptr;

    WGPUCommandEncoder commandEncoder = wgpuDeviceCreateCommandEncoder(device_, &commandEncoderDescriptor);

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

    WGPURenderPassColorAttachment colorAttachment;
    colorAttachment.nextInChain = nullptr;
    colorAttachment.view = surfaceTextureView;
    colorAttachment.resolveTarget = nullptr;
    colorAttachment.loadOp = WGPULoadOp_Clear;
    colorAttachment.storeOp = WGPUStoreOp_Store;
    colorAttachment.clearValue = {0.6, 0.8, 1.0, 0.0};

    WGPURenderPassDepthStencilAttachment depthStencilAttachment;
    depthStencilAttachment.view = depthTextureView_;
    depthStencilAttachment.depthLoadOp = WGPULoadOp_Clear;
    depthStencilAttachment.depthStoreOp = WGPUStoreOp_Store;
    depthStencilAttachment.depthClearValue = 1.f;
    depthStencilAttachment.depthReadOnly = false;
    depthStencilAttachment.stencilLoadOp = WGPULoadOp_Clear;
    depthStencilAttachment.stencilStoreOp = WGPUStoreOp_Discard;
    depthStencilAttachment.stencilClearValue = 0;
    depthStencilAttachment.stencilReadOnly = true;

    WGPURenderPassDescriptor renderPassDescriptor;
    renderPassDescriptor.nextInChain = nullptr;
    renderPassDescriptor.label = "main";
    renderPassDescriptor.colorAttachmentCount = 1;
    renderPassDescriptor.colorAttachments = &colorAttachment;
    renderPassDescriptor.depthStencilAttachment = &depthStencilAttachment;
    renderPassDescriptor.occlusionQuerySet = nullptr;
    renderPassDescriptor.timestampWrites = nullptr;

    WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);

    wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 0, camera_.bindGroup(), 0, nullptr);
    wgpuRenderPassEncoderSetPipeline(renderPassEncoder, previewPipeline_.renderPipeline());
    wgpuRenderPassEncoderSetVertexBuffer(renderPassEncoder, 0, sceneData.vertexBuffer(), 0, wgpuBufferGetSize(sceneData.vertexBuffer()));
    wgpuRenderPassEncoderSetIndexBuffer(renderPassEncoder, sceneData.indexBuffer(), WGPUIndexFormat_Uint32, 0, wgpuBufferGetSize(sceneData.indexBuffer()));
    wgpuRenderPassEncoderDrawIndexed(renderPassEncoder, sceneData.indexCount(), 1, 0, 0, 0);
    wgpuRenderPassEncoderEnd(renderPassEncoder);

    WGPUCommandBufferDescriptor commandBufferDescriptor;
    commandBufferDescriptor.nextInChain = nullptr;
    commandBufferDescriptor.label = nullptr;

    WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(commandEncoder, &commandBufferDescriptor);

    wgpuQueueSubmit(queue_, 1, &commandBuffer);

    wgpuCommandBufferRelease(commandBuffer);
    wgpuRenderPassEncoderRelease(renderPassEncoder);
    wgpuTextureViewRelease(surfaceTextureView);
    wgpuCommandEncoderRelease(commandEncoder);
}

Renderer::Renderer(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat surfaceFormat, ShaderRegistry & shaderRegistry)
    : pimpl_(std::make_unique<Impl>(device, queue, surfaceFormat, shaderRegistry))
{}

// Need to explicitly implement the destructor for pimpl to work
// See https://www.fluentcpp.com/2017/09/22/make-pimpl-using-unique_ptr
Renderer::~Renderer() = default;

void Renderer::renderFrame(WGPUTexture surfaceTexture, Camera const & camera, SceneData const & sceneData)
{
    pimpl_->renderFrame(surfaceTexture, camera, sceneData);
}
