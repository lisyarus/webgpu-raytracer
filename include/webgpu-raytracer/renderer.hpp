#pragma once

#include <webgpu.h>

#include <memory>

struct Renderer
{
    Renderer(WGPUDevice device, WGPUQueue queue, WGPUTextureFormat surfaceFormat);
    ~Renderer();

    void renderFrame(WGPUTexture surfaceTexture);

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};
