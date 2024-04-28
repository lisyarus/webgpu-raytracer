#include <webgpu-raytracer/raytrace_first_hit_pipeline.hpp>

RaytraceFirstHitPipeline::RaytraceFirstHitPipeline(WGPUDevice device, ShaderRegistry & shaderRegistry, WGPUTextureFormat surfaceFormat,
    WGPUBindGroupLayout cameraBindGroupLayout, WGPUBindGroupLayout geometryBindGroupLayout, WGPUBindGroupLayout materialBindGroupLayout)
{
    WGPUBindGroupLayout bindGroupLayouts[3]
    {
        cameraBindGroupLayout,
        geometryBindGroupLayout,
        materialBindGroupLayout,
    };

    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor;
    pipelineLayoutDescriptor.nextInChain = nullptr;
    pipelineLayoutDescriptor.label = nullptr;
    pipelineLayoutDescriptor.bindGroupLayoutCount = 3;
    pipelineLayoutDescriptor.bindGroupLayouts = bindGroupLayouts;

    pipelineLayout_ = wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDescriptor);

    WGPUShaderModule shaderModule = shaderRegistry.loadShaderModule("raytrace_first_hit");

    WGPUBlendState blendState;
    blendState.color.operation = WGPUBlendOperation_Add;
    blendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
    blendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    blendState.alpha.operation = WGPUBlendOperation_Add;
    blendState.alpha.srcFactor = WGPUBlendFactor_SrcAlpha;
    blendState.alpha.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;

    WGPUColorTargetState colorTarget;
    colorTarget.nextInChain = nullptr;
    colorTarget.format = surfaceFormat;
    colorTarget.blend = &blendState;
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
    renderPipelineDescriptor.vertex.bufferCount = 0;
    renderPipelineDescriptor.vertex.buffers = nullptr;
    renderPipelineDescriptor.primitive.nextInChain = nullptr;
    renderPipelineDescriptor.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    renderPipelineDescriptor.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
    renderPipelineDescriptor.primitive.frontFace = WGPUFrontFace_CCW;
    renderPipelineDescriptor.primitive.cullMode = WGPUCullMode_None;
    renderPipelineDescriptor.depthStencil = nullptr;
    renderPipelineDescriptor.multisample.nextInChain = nullptr;
    renderPipelineDescriptor.multisample.count = 1;
    renderPipelineDescriptor.multisample.mask = 1;
    renderPipelineDescriptor.multisample.alphaToCoverageEnabled = false;
    renderPipelineDescriptor.fragment = &fragmentState;

    renderPipeline_ = wgpuDeviceCreateRenderPipeline(device, &renderPipelineDescriptor);
}

RaytraceFirstHitPipeline::~RaytraceFirstHitPipeline()
{
    wgpuRenderPipelineRelease(renderPipeline_);
    wgpuPipelineLayoutRelease(pipelineLayout_);
}

void renderRaytraceFirstHit(WGPUCommandEncoder commandEncoder, WGPUTextureView colorTextureView, bool clear,
    WGPURenderPipeline raytraceFirstHitPipeline, WGPUBindGroup cameraBindGroup, SceneData const & sceneData)
{
    WGPURenderPassColorAttachment colorAttachment;
    colorAttachment.nextInChain = nullptr;
    colorAttachment.view = colorTextureView;
    colorAttachment.resolveTarget = nullptr;
    colorAttachment.loadOp = clear ? WGPULoadOp_Clear : WGPULoadOp_Load;
    colorAttachment.storeOp = WGPUStoreOp_Store;
    colorAttachment.clearValue = {0.0, 0.0, 0.0, 0.0};

    WGPURenderPassDescriptor renderPassDescriptor;
    renderPassDescriptor.nextInChain = nullptr;
    renderPassDescriptor.label = "preview";
    renderPassDescriptor.colorAttachmentCount = 1;
    renderPassDescriptor.colorAttachments = &colorAttachment;
    renderPassDescriptor.depthStencilAttachment = nullptr;
    renderPassDescriptor.occlusionQuerySet = nullptr;
    renderPassDescriptor.timestampWrites = nullptr;

    WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);

    wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 0, cameraBindGroup, 0, nullptr);
    wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 1, sceneData.geometryBindGroup(), 0, nullptr);
    wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 2, sceneData.materialBindGroup(), 0, nullptr);
    wgpuRenderPassEncoderSetPipeline(renderPassEncoder, raytraceFirstHitPipeline);
    wgpuRenderPassEncoderDraw(renderPassEncoder, 3, 1, 0, 0);
    wgpuRenderPassEncoderEnd(renderPassEncoder);
    wgpuRenderPassEncoderRelease(renderPassEncoder);
}
