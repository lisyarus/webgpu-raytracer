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

private:
    WGPUPipelineLayout pipelineLayout_;
    WGPURenderPipeline renderPipeline_;
};

void renderPreview(WGPUCommandEncoder commandEncoder, WGPUTextureView colorTextureView, WGPUTextureView depthTextureView,
    WGPURenderPipeline previewPipeline, WGPUBindGroup cameraBindGroup, SceneData const & sceneData, glm::vec3 const & backgroundColor);
