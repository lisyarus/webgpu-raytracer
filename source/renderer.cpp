#include <webgpu-raytracer/renderer.hpp>

struct Renderer::Impl
{
    Impl(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat surfaceFormat);

    void renderFrame(WGPUTexture surfaceTexture);

private:

    WGPUDevice device_;
    WGPUQueue queue_;
    WGPUTextureFormat surfaceFormat_;
};

Renderer::Impl::Impl(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat surfaceFormat)
    : device_(device)
    , queue_(queue)
    , surfaceFormat_(surfaceFormat)
{}

void Renderer::Impl::renderFrame(WGPUTexture surfaceTexture)
{
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

    WGPURenderPassDescriptor renderPassDescriptor;
    renderPassDescriptor.nextInChain = nullptr;
    renderPassDescriptor.label = nullptr;
    renderPassDescriptor.colorAttachmentCount = 1;
    renderPassDescriptor.colorAttachments = &colorAttachment;
    renderPassDescriptor.depthStencilAttachment = nullptr;
    renderPassDescriptor.occlusionQuerySet = nullptr;
    renderPassDescriptor.timestampWrites = nullptr;

    WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);
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

Renderer::Renderer(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat surfaceFormat)
    : pimpl_(std::make_unique<Impl>(device, queue, surfaceFormat))
{}

// Need to explicitly implement the destructor for pimpl to work
// See https://www.fluentcpp.com/2017/09/22/make-pimpl-using-unique_ptr
Renderer::~Renderer() = default;

void Renderer::renderFrame(WGPUTexture surfaceTexture)
{
    pimpl_->renderFrame(surfaceTexture);
}
