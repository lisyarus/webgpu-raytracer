#include <webgpu-raytracer/raytrace_first_hit_pipeline.hpp>

RaytraceFirstHitPipeline::RaytraceFirstHitPipeline(WGPUDevice device, ShaderRegistry & shaderRegistry, WGPUBindGroupLayout cameraBindGroupLayout,
    WGPUBindGroupLayout geometryBindGroupLayout, WGPUBindGroupLayout materialBindGroupLayout, WGPUBindGroupLayout accumulationStorageBindGroupLayout)
{
    WGPUBindGroupLayout bindGroupLayouts[4]
    {
        cameraBindGroupLayout,
        geometryBindGroupLayout,
        materialBindGroupLayout,
        accumulationStorageBindGroupLayout,
    };

    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor;
    pipelineLayoutDescriptor.nextInChain = nullptr;
    pipelineLayoutDescriptor.label = nullptr;
    pipelineLayoutDescriptor.bindGroupLayoutCount = 4;
    pipelineLayoutDescriptor.bindGroupLayouts = bindGroupLayouts;

    pipelineLayout_ = wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDescriptor);

    WGPUShaderModule shaderModule = shaderRegistry.loadShaderModule("raytrace_first_hit");

    WGPUComputePipelineDescriptor pipelineDescriptor;
    pipelineDescriptor.nextInChain = nullptr;
    pipelineDescriptor.label = "raytrace_first_hit";
    pipelineDescriptor.layout = pipelineLayout_;
    pipelineDescriptor.compute.nextInChain = nullptr;
    pipelineDescriptor.compute.module = shaderModule;
    pipelineDescriptor.compute.entryPoint = "computeMain";
    pipelineDescriptor.compute.constantCount = 0;
    pipelineDescriptor.compute.constants = nullptr;

    pipeline_ = wgpuDeviceCreateComputePipeline(device, &pipelineDescriptor);
}

RaytraceFirstHitPipeline::~RaytraceFirstHitPipeline()
{
    wgpuComputePipelineRelease(pipeline_);
    wgpuPipelineLayoutRelease(pipelineLayout_);
}

void renderRaytraceFirstHit(WGPUCommandEncoder commandEncoder, WGPUTextureView colorTextureView, WGPUComputePipeline raytraceMonteCarloPipeline,
    WGPUBindGroup cameraBindGroup, SceneData const & sceneData, WGPUBindGroup accumulationStorageBindGroup, glm::uvec2 const & screenSize)
{
    WGPUComputePassDescriptor computePassDescriptor;
    computePassDescriptor.nextInChain = nullptr;
    computePassDescriptor.label = "raytrace_first_hit";
    computePassDescriptor.timestampWrites = nullptr;

    WGPUComputePassEncoder computePassEncoder = wgpuCommandEncoderBeginComputePass(commandEncoder, &computePassDescriptor);

    wgpuComputePassEncoderSetBindGroup(computePassEncoder, 0, cameraBindGroup, 0, nullptr);
    wgpuComputePassEncoderSetBindGroup(computePassEncoder, 1, sceneData.geometryBindGroup(), 0, nullptr);
    wgpuComputePassEncoderSetBindGroup(computePassEncoder, 2, sceneData.materialBindGroup(), 0, nullptr);
    wgpuComputePassEncoderSetBindGroup(computePassEncoder, 3, accumulationStorageBindGroup, 0, nullptr);
    wgpuComputePassEncoderSetPipeline(computePassEncoder, raytraceMonteCarloPipeline);
    wgpuComputePassEncoderDispatchWorkgroups(computePassEncoder, (screenSize.x + 7) / 8, (screenSize.y + 7) / 8, 1);
    wgpuComputePassEncoderEnd(computePassEncoder);
    wgpuComputePassEncoderRelease(computePassEncoder);
}
