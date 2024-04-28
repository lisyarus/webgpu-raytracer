#pragma once

#include <webgpu-raytracer/shader_registry.hpp>

#include <webgpu.h>

struct ComposePipeline
{
    ComposePipeline(WGPUDevice device, ShaderRegistry & shaderRegistry, WGPUTextureFormat surfaceFormat,
        WGPUBindGroupLayout accumulationBindGroupLayout);
    ~ComposePipeline();

    WGPURenderPipeline renderPipeline() const { return renderPipeline_; }

private:
    WGPUPipelineLayout pipelineLayout_;
    WGPURenderPipeline renderPipeline_;
};

void renderCompose(WGPUCommandEncoder commandEncoder, WGPUTextureView colorTextureView, WGPURenderPipeline composePipeline, WGPUBindGroup accumulationBindGroup);
