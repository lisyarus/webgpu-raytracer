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

    WGPUVertexAttribute vertexAttributes[3];
    vertexAttributes[0].format = WGPUVertexFormat_Float32x3;
    vertexAttributes[0].offset = 0;
    vertexAttributes[0].shaderLocation = 1;
    vertexAttributes[1].format = WGPUVertexFormat_Uint32;
    vertexAttributes[1].offset = 12;
    vertexAttributes[1].shaderLocation = 2;
    vertexAttributes[2].format = WGPUVertexFormat_Float32x2;
    vertexAttributes[2].offset = 16;
    vertexAttributes[2].shaderLocation = 3;

    WGPUVertexBufferLayout vertexBufferLayouts[2];
    vertexBufferLayouts[0].arrayStride = 16;
    vertexBufferLayouts[0].stepMode = WGPUVertexStepMode_Vertex;
    vertexBufferLayouts[0].attributeCount = 1;
    vertexBufferLayouts[0].attributes = vertexPositionAttributes;
    vertexBufferLayouts[1].arrayStride = 32;
    vertexBufferLayouts[1].stepMode = WGPUVertexStepMode_Vertex;
    vertexBufferLayouts[1].attributeCount = 3;
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

    WGPUDepthStencilState backgroundDepthStencilState;
    backgroundDepthStencilState.nextInChain = nullptr;
    backgroundDepthStencilState.format = WGPUTextureFormat_Depth24Plus;
    backgroundDepthStencilState.depthWriteEnabled = false;
    backgroundDepthStencilState.depthCompare = WGPUCompareFunction_Always;
    backgroundDepthStencilState.stencilFront.compare = WGPUCompareFunction_Always;
    backgroundDepthStencilState.stencilFront.failOp = WGPUStencilOperation_Keep;
    backgroundDepthStencilState.stencilFront.depthFailOp = WGPUStencilOperation_Keep;
    backgroundDepthStencilState.stencilFront.passOp = WGPUStencilOperation_Keep;
    backgroundDepthStencilState.stencilBack.compare = WGPUCompareFunction_Always;
    backgroundDepthStencilState.stencilBack.failOp = WGPUStencilOperation_Keep;
    backgroundDepthStencilState.stencilBack.depthFailOp = WGPUStencilOperation_Keep;
    backgroundDepthStencilState.stencilBack.passOp = WGPUStencilOperation_Keep;
    backgroundDepthStencilState.stencilReadMask = 0;
    backgroundDepthStencilState.stencilWriteMask = 0;
    backgroundDepthStencilState.depthBias = 0;
    backgroundDepthStencilState.depthBiasSlopeScale = 0.f;
    backgroundDepthStencilState.depthBiasClamp = 0.f;

    WGPUFragmentState backgroundFragmentState;
    backgroundFragmentState.nextInChain = nullptr;
    backgroundFragmentState.module = shaderModule;
    backgroundFragmentState.entryPoint = "backgroundFragmentMain";
    backgroundFragmentState.constantCount = 0;
    backgroundFragmentState.constants = nullptr;
    backgroundFragmentState.targetCount = 1;
    backgroundFragmentState.targets = &colorTarget;

    WGPURenderPipelineDescriptor backgroundPipelineDescriptor;
    backgroundPipelineDescriptor.nextInChain = nullptr;
    backgroundPipelineDescriptor.label = "preview_background";
    backgroundPipelineDescriptor.layout = pipelineLayout_;
    backgroundPipelineDescriptor.vertex.nextInChain = nullptr;
    backgroundPipelineDescriptor.vertex.module = shaderModule;
    backgroundPipelineDescriptor.vertex.entryPoint = "backgroundVertexMain";
    backgroundPipelineDescriptor.vertex.constantCount = 0;
    backgroundPipelineDescriptor.vertex.constants = nullptr;
    backgroundPipelineDescriptor.vertex.bufferCount = 0;
    backgroundPipelineDescriptor.vertex.buffers = nullptr;
    backgroundPipelineDescriptor.primitive.nextInChain = nullptr;
    backgroundPipelineDescriptor.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    backgroundPipelineDescriptor.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
    backgroundPipelineDescriptor.primitive.frontFace = WGPUFrontFace_CCW;
    backgroundPipelineDescriptor.primitive.cullMode = WGPUCullMode_None;
    backgroundPipelineDescriptor.depthStencil = &backgroundDepthStencilState;
    backgroundPipelineDescriptor.multisample.nextInChain = nullptr;
    backgroundPipelineDescriptor.multisample.count = 1;
    backgroundPipelineDescriptor.multisample.mask = 1;
    backgroundPipelineDescriptor.multisample.alphaToCoverageEnabled = false;
    backgroundPipelineDescriptor.fragment = &backgroundFragmentState;

    renderPipeline_ = wgpuDeviceCreateRenderPipeline(device, &renderPipelineDescriptor);
    backgroundPipeline_ = wgpuDeviceCreateRenderPipeline(device, &backgroundPipelineDescriptor);
}

PreviewPipeline::~PreviewPipeline()
{
    wgpuRenderPipelineRelease(renderPipeline_);
    wgpuPipelineLayoutRelease(pipelineLayout_);
}

void renderPreview(PreviewPipeline const & previewPipeline, WGPUCommandEncoder commandEncoder, WGPUTextureView colorTextureView, WGPUTextureView depthTextureView,
    WGPUBindGroup cameraBindGroup, SceneData const & sceneData)
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
    wgpuRenderPassEncoderSetPipeline(renderPassEncoder, previewPipeline.backgroundPipeline());
    wgpuRenderPassEncoderDraw(renderPassEncoder, 3, 1, 0, 0);
    wgpuRenderPassEncoderSetPipeline(renderPassEncoder, previewPipeline.renderPipeline());
    wgpuRenderPassEncoderSetVertexBuffer(renderPassEncoder, 0, sceneData.vertexPositionsBuffer(), 0, wgpuBufferGetSize(sceneData.vertexPositionsBuffer()));
    wgpuRenderPassEncoderSetVertexBuffer(renderPassEncoder, 1, sceneData.vertexAttributesBuffer(), 0, wgpuBufferGetSize(sceneData.vertexAttributesBuffer()));
    wgpuRenderPassEncoderDraw(renderPassEncoder, sceneData.vertexCount(), 1, 0, 0);
    wgpuRenderPassEncoderEnd(renderPassEncoder);
    wgpuRenderPassEncoderRelease(renderPassEncoder);
}
