#include <webgpu-raytracer/preview_pipeline.hpp>

PreviewPipeline::PreviewPipeline(WGPUDevice device, ShaderRegistry & shaderRegistry, WGPUTextureFormat surfaceFormat,
    WGPUBindGroupLayout cameraBindGroupLayout, WGPUBindGroupLayout materialBindGroupLayout)
{
    WGPUBindGroupLayout bindGroupLayouts[2]
    {
        cameraBindGroupLayout,
        materialBindGroupLayout,
    };

    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor;
    pipelineLayoutDescriptor.nextInChain = nullptr;
    pipelineLayoutDescriptor.label = nullptr;
    pipelineLayoutDescriptor.bindGroupLayoutCount = 2;
    pipelineLayoutDescriptor.bindGroupLayouts = bindGroupLayouts;

    pipelineLayout_ = wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDescriptor);

    WGPUShaderModule shaderModule = shaderRegistry.loadShaderModule("preview");

    WGPUVertexAttribute vertexPositionAttributes[1];
    vertexPositionAttributes[0].format = WGPUVertexFormat_Float32x3;
    vertexPositionAttributes[0].offset = 0;
    vertexPositionAttributes[0].shaderLocation = 0;

    WGPUVertexAttribute vertexAttributes[2];
    vertexAttributes[0].format = WGPUVertexFormat_Float32x3;
    vertexAttributes[0].offset = 0;
    vertexAttributes[0].shaderLocation = 1;
    vertexAttributes[1].format = WGPUVertexFormat_Uint32;
    vertexAttributes[1].offset = 12;
    vertexAttributes[1].shaderLocation = 2;

    WGPUVertexBufferLayout vertexBufferLayouts[2];
    vertexBufferLayouts[0].arrayStride = 16;
    vertexBufferLayouts[0].stepMode = WGPUVertexStepMode_Vertex;
    vertexBufferLayouts[0].attributeCount = 1;
    vertexBufferLayouts[0].attributes = vertexPositionAttributes;
    vertexBufferLayouts[1].arrayStride = 16;
    vertexBufferLayouts[1].stepMode = WGPUVertexStepMode_Vertex;
    vertexBufferLayouts[1].attributeCount = 2;
    vertexBufferLayouts[1].attributes = vertexAttributes;

    WGPUDepthStencilState depthStencilState;
    depthStencilState.nextInChain = nullptr;
    depthStencilState.format = WGPUTextureFormat_Depth24Plus;
    depthStencilState.depthWriteEnabled = true;
    depthStencilState.depthCompare = WGPUCompareFunction_Less;
    depthStencilState.stencilFront.compare = WGPUCompareFunction_Always;
    depthStencilState.stencilFront.failOp = WGPUStencilOperation_Keep;
    depthStencilState.stencilFront.depthFailOp = WGPUStencilOperation_Keep;
    depthStencilState.stencilFront.passOp = WGPUStencilOperation_Keep;
    depthStencilState.stencilBack.compare = WGPUCompareFunction_Always;
    depthStencilState.stencilBack.failOp = WGPUStencilOperation_Keep;
    depthStencilState.stencilBack.depthFailOp = WGPUStencilOperation_Keep;
    depthStencilState.stencilBack.passOp = WGPUStencilOperation_Keep;
    depthStencilState.stencilReadMask = 0;
    depthStencilState.stencilWriteMask = 0;
    depthStencilState.depthBias = 0;
    depthStencilState.depthBiasSlopeScale = 0.f;
    depthStencilState.depthBiasClamp = 0.f;

    WGPUColorTargetState colorTarget;
    colorTarget.nextInChain = nullptr;
    colorTarget.format = surfaceFormat;
    colorTarget.blend = nullptr;
    colorTarget.writeMask = WGPUColorWriteMask_All;

    WGPUFragmentState fragmentState;
    fragmentState.nextInChain = nullptr;
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = "fragmentMain";
    fragmentState.constantCount = 0;
    fragmentState.constants = nullptr;
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;

    WGPURenderPipelineDescriptor renderPipelineDescriptor;
    renderPipelineDescriptor.nextInChain = nullptr;
    renderPipelineDescriptor.label = "preview";
    renderPipelineDescriptor.layout = pipelineLayout_;
    renderPipelineDescriptor.vertex.nextInChain = nullptr;
    renderPipelineDescriptor.vertex.module = shaderModule;
    renderPipelineDescriptor.vertex.entryPoint = "vertexMain";
    renderPipelineDescriptor.vertex.constantCount = 0;
    renderPipelineDescriptor.vertex.constants = nullptr;
    renderPipelineDescriptor.vertex.bufferCount = 2;
    renderPipelineDescriptor.vertex.buffers = vertexBufferLayouts;
    renderPipelineDescriptor.primitive.nextInChain = nullptr;
    renderPipelineDescriptor.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    renderPipelineDescriptor.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
    renderPipelineDescriptor.primitive.frontFace = WGPUFrontFace_CCW;
    renderPipelineDescriptor.primitive.cullMode = WGPUCullMode_None;
    renderPipelineDescriptor.depthStencil = &depthStencilState;
    renderPipelineDescriptor.multisample.nextInChain = nullptr;
    renderPipelineDescriptor.multisample.count = 1;
    renderPipelineDescriptor.multisample.mask = 1;
    renderPipelineDescriptor.multisample.alphaToCoverageEnabled = false;
    renderPipelineDescriptor.fragment = &fragmentState;

    renderPipeline_ = wgpuDeviceCreateRenderPipeline(device, &renderPipelineDescriptor);
}

PreviewPipeline::~PreviewPipeline()
{
    wgpuRenderPipelineRelease(renderPipeline_);
    wgpuPipelineLayoutRelease(pipelineLayout_);
}

void renderPreview(WGPUCommandEncoder commandEncoder, WGPUTextureView colorTextureView, WGPUTextureView depthTextureView,
    WGPURenderPipeline previewPipeline, WGPUBindGroup cameraBindGroup, SceneData const & sceneData)
{
    WGPURenderPassColorAttachment colorAttachment;
    colorAttachment.nextInChain = nullptr;
    colorAttachment.view = colorTextureView;
    colorAttachment.resolveTarget = nullptr;
    colorAttachment.loadOp = WGPULoadOp_Clear;
    colorAttachment.storeOp = WGPUStoreOp_Store;
    colorAttachment.clearValue = {0.6, 0.8, 1.0, 0.0};

    WGPURenderPassDepthStencilAttachment depthStencilAttachment;
    depthStencilAttachment.view = depthTextureView;
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
    renderPassDescriptor.label = "preview";
    renderPassDescriptor.colorAttachmentCount = 1;
    renderPassDescriptor.colorAttachments = &colorAttachment;
    renderPassDescriptor.depthStencilAttachment = &depthStencilAttachment;
    renderPassDescriptor.occlusionQuerySet = nullptr;
    renderPassDescriptor.timestampWrites = nullptr;

    WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);

    wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 0, cameraBindGroup, 0, nullptr);
    wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 1, sceneData.materialBindGroup(), 0, nullptr);
    wgpuRenderPassEncoderSetPipeline(renderPassEncoder, previewPipeline);
    wgpuRenderPassEncoderSetVertexBuffer(renderPassEncoder, 0, sceneData.vertexPositionsBuffer(), 0, wgpuBufferGetSize(sceneData.vertexPositionsBuffer()));
    wgpuRenderPassEncoderSetVertexBuffer(renderPassEncoder, 1, sceneData.vertexAttributesBuffer(), 0, wgpuBufferGetSize(sceneData.vertexAttributesBuffer()));
    wgpuRenderPassEncoderDraw(renderPassEncoder, sceneData.vertexCount(), 1, 0, 0);
    wgpuRenderPassEncoderEnd(renderPassEncoder);
    wgpuRenderPassEncoderRelease(renderPassEncoder);
}
