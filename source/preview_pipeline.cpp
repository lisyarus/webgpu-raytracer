#include <webgpu-raytracer/preview_pipeline.hpp>

PreviewPipeline::PreviewPipeline(WGPUDevice device, ShaderRegistry & shaderRegistry, WGPUTextureFormat surfaceFormat, WGPUBindGroupLayout cameraBindGroupLayout)
{
    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor;
    pipelineLayoutDescriptor.nextInChain = nullptr;
    pipelineLayoutDescriptor.label = nullptr;
    pipelineLayoutDescriptor.bindGroupLayoutCount = 1;
    pipelineLayoutDescriptor.bindGroupLayouts = &cameraBindGroupLayout;

    pipelineLayout_ = wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDescriptor);

    WGPUShaderModule shaderModule = shaderRegistry.loadShaderModule("preview");

    WGPUVertexAttribute vertexAttributes[2];
    vertexAttributes[0].format = WGPUVertexFormat_Float32x3;
    vertexAttributes[0].offset = 0;
    vertexAttributes[0].shaderLocation = 0;
    vertexAttributes[1].format = WGPUVertexFormat_Float32x3;
    vertexAttributes[1].offset = 12;
    vertexAttributes[1].shaderLocation = 1;

    WGPUVertexBufferLayout vertexBufferLayout;
    vertexBufferLayout.arrayStride = 24;
    vertexBufferLayout.stepMode = WGPUVertexStepMode_Vertex;
    vertexBufferLayout.attributeCount = 2;
    vertexBufferLayout.attributes = vertexAttributes;

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
    renderPipelineDescriptor.vertex.bufferCount = 1;
    renderPipelineDescriptor.vertex.buffers = &vertexBufferLayout;
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
