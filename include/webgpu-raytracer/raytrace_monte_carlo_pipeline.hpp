#pragma once

#include <webgpu-raytracer/shader_registry.hpp>
#include <webgpu-raytracer/scene_data.hpp>
#include <webgpu-raytracer/camera_bind_group.hpp>

#include <webgpu.h>

struct RaytraceMonteCarloPipeline
{
    RaytraceMonteCarloPipeline(WGPUDevice device, ShaderRegistry & shaderRegistry, WGPUTextureFormat surfaceFormat,
        WGPUBindGroupLayout cameraBindGroupLayout, WGPUBindGroupLayout geometryBindGroupLayout, WGPUBindGroupLayout materialBindGroupLayout);
    ~RaytraceMonteCarloPipeline();

    WGPURenderPipeline renderPipeline() const { return renderPipeline_; }

private:
    WGPUPipelineLayout pipelineLayout_;
    WGPURenderPipeline renderPipeline_;
};

void renderRaytraceMonteCarlo(WGPUCommandEncoder commandEncoder, WGPUTextureView colorTextureView, bool clear,
    WGPURenderPipeline raytraceMonteCarloPipeline, WGPUBindGroup cameraBindGroup, SceneData const & sceneData);
