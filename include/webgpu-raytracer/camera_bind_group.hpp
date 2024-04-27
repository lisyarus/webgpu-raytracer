#pragma once

#include <webgpu-raytracer/camera.hpp>

#include <webgpu.h>

struct CameraBindGroup
{
    CameraBindGroup(WGPUDevice device);
    ~CameraBindGroup();

    void update(WGPUQueue queue, Camera const & camera);

    WGPUBindGroupLayout bindGroupLayout() const { return bindGroupLayout_; }
    WGPUBindGroup bindGroup() const { return bindGroup_; }

private:
    WGPUBuffer uniformBuffer_;
    WGPUBindGroupLayout bindGroupLayout_;
    WGPUBindGroup bindGroup_;
};
