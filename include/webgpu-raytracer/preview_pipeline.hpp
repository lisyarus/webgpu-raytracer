#pragma once

#include <webgpu-raytracer/shader_registry.hpp>

#include <webgpu.h>

struct PreviewPipeline
{
    PreviewPipeline(WGPUDevice device, ShaderRegistry & shaderRegistry, WGPUTextureFormat surfaceFormat, WGPUBindGroupLayout cameraBindGroupLayout);
    ~PreviewPipeline();

    WGPURenderPipeline renderPipeline() const { return renderPipeline_; }

private:
    WGPUPipelineLayout pipelineLayout_;
    WGPURenderPipeline renderPipeline_;
};
