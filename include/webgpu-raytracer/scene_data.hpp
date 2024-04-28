#pragma once

#include <webgpu-raytracer/gltf_asset.hpp>

#include <webgpu.h>

struct SceneData
{
    SceneData(glTF::Asset const & asset, WGPUDevice device, WGPUQueue queue,
        WGPUBindGroupLayout geometryBindGroupLayout, WGPUBindGroupLayout materialBindGroupLayout);
    ~SceneData();

    WGPUBuffer vertexBuffer() const { return vertexBuffer_; }
    WGPUBuffer indexBuffer() const { return indexBuffer_; }
    WGPUBuffer materialBuffer() const { return materialBuffer_; }

    std::uint32_t indexCount() const { return indexCount_; }

    WGPUBindGroup geometryBindGroup() const { return geometryBindGroup_; }
    WGPUBindGroup materialBindGroup() const { return materialBindGroup_; }

private:
    WGPUBuffer vertexBuffer_;
    WGPUBuffer indexBuffer_;
    WGPUBuffer materialBuffer_;

    std::uint32_t indexCount_;

    WGPUBindGroup geometryBindGroup_;
    WGPUBindGroup materialBindGroup_;
};
