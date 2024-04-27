#pragma once

#include <webgpu-raytracer/gltf_asset.hpp>

#include <webgpu.h>

struct SceneData
{
    SceneData(glTF::Asset const & asset, WGPUDevice device, WGPUQueue queue);
    ~SceneData();

    WGPUBuffer vertexBuffer() const { return vertexBuffer_; }
    WGPUBuffer indexBuffer() const { return indexBuffer_; }

    std::uint32_t indexCount() const { return indexCount_; }

private:
    WGPUBuffer vertexBuffer_;
    WGPUBuffer indexBuffer_;

    std::uint32_t indexCount_;
};
