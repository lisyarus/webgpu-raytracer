#pragma once

#include <webgpu-raytracer/gltf_asset.hpp>

#include <webgpu.h>

struct SceneData
{
    SceneData(glTF::Asset const & asset, WGPUDevice device, WGPUQueue queue,
        WGPUBindGroupLayout geometryBindGroupLayout, WGPUBindGroupLayout materialBindGroupLayout);
    ~SceneData();

    WGPUBuffer vertexPositionsBuffer() const { return vertexPositionsBuffer_; }
    WGPUBuffer vertexAttributesBuffer() const { return vertexAttributesBuffer_; }

    std::uint32_t vertexCount() const { return vertexCount_; }

    WGPUBindGroup geometryBindGroup() const { return geometryBindGroup_; }
    WGPUBindGroup materialBindGroup() const { return materialBindGroup_; }

private:
    WGPUBuffer vertexPositionsBuffer_;
    WGPUBuffer vertexAttributesBuffer_;
    WGPUBuffer materialBuffer_;
    WGPUBuffer bvhNodesBuffer_;
    WGPUBuffer emissiveTrianglesBuffer_;
    WGPUBuffer emissiveBvhNodesBuffer_;

    std::uint32_t vertexCount_;

    WGPUBindGroup geometryBindGroup_;
    WGPUBindGroup materialBindGroup_;
};
