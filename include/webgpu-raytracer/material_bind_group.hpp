#pragma once

#include <webgpu.h>

WGPUBindGroupLayout createMaterialBindGroupLayout(WGPUDevice device);

WGPUBindGroup createMaterialBindGroup(WGPUDevice device, WGPUBindGroupLayout bindGroupLayout, WGPUBuffer materialBuffer, WGPUSampler textureSampler,
    WGPUTextureView albedoTexture, WGPUTextureView environmentTexture);
