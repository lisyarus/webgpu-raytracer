#pragma once

#include <webgpu-raytracer/gltf_asset.hpp>

#include <webgpu.h>

struct SceneData
{
    SceneData(glTF::Asset const & asset, WGPUDevice device, WGPUQueue queue);

    WGPUBuffer vertexBuffer() const { return vertexBuffer_; }
    WGPUBuffer indexBuffer() const { return indexBuffer_; }

private:
    WGPUBuffer vertexBuffer_;
    WGPUBuffer indexBuffer_;
};
