#pragma once

#include <webgpu-raytracer/camera.hpp>

#include <webgpu.h>

struct ComposeUniformsBindGroup
{
    ComposeUniformsBindGroup(WGPUDevice device);
    ~ComposeUniformsBindGroup();

    void update(WGPUQueue queue, float exposure);

    WGPUBindGroupLayout bindGroupLayout() const { return bindGroupLayout_; }
    WGPUBindGroup bindGroup() const { return bindGroup_; }

private:
    WGPUBuffer uniformBuffer_;
    WGPUBindGroupLayout bindGroupLayout_;
    WGPUBindGroup bindGroup_;
};
