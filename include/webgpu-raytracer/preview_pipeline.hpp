#pragma once

#include <webgpu-raytracer/shader_registry.hpp>
#include <webgpu-raytracer/scene_data.hpp>
#include <webgpu-raytracer/camera_bind_group.hpp>

#include <webgpu.h>

struct PreviewPipeline
{
    PreviewPipeline(WGPUDevice device, ShaderRegistry & shaderRegistry, WGPUTextureFormat surfaceFormat,
        WGPUBindGroupLayout cameraBindGroupLayout, WGPUBindGroupLayout materialBindGroupLayout);
    ~PreviewPipeline();

    WGPURenderPipeline renderPipeline() const { return renderPipeline_; }
    WGPURenderPipeline backgroundPipeline() const { return backgroundPipeline_; }

private:
    WGPUPipelineLayout pipelineLayout_;
    WGPURenderPipeline renderPipeline_;
    WGPURenderPipeline backgroundPipeline_;
};

void renderPreview(PreviewPipeline const & previewPipeline, WGPUCommandEncoder commandEncoder, WGPUTextureView colorTextureView, WGPUTextureView depthTextureView,
    WGPUBindGroup cameraBindGroup, SceneData const & sceneData);
