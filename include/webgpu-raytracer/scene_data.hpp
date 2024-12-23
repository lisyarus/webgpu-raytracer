#pragma once

#include <webgpu-raytracer/gltf_asset.hpp>

#include <webgpu.h>

struct HDRIData
{
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<float> pixels;
};

struct SceneData
{
    SceneData(glTF::Asset const & asset, HDRIData const & environmentMap, WGPUDevice device, WGPUQueue queue,
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

    WGPUSampler sampler_;

    WGPUTexture albedoTexture_;
    WGPUTextureView albedoTextureView_;

    WGPUTexture materialTexture_;
    WGPUTextureView materialTextureView_;

    WGPUTexture environmentTexture_;
    WGPUTextureView environmentTextureView_;

    WGPUBindGroup geometryBindGroup_;
    WGPUBindGroup materialBindGroup_;
};
