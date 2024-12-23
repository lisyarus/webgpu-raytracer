#include <webgpu-raytracer/material_bind_group.hpp>

WGPUBindGroupLayout createMaterialBindGroupLayout(WGPUDevice device)
{
    WGPUBindGroupLayoutEntry layoutEntries[4];

    layoutEntries[0].nextInChain = nullptr;
    layoutEntries[0].binding = 0;
    layoutEntries[0].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment | WGPUShaderStage_Compute;
    layoutEntries[0].buffer.nextInChain = nullptr;
    layoutEntries[0].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
    layoutEntries[0].buffer.hasDynamicOffset = false;
    layoutEntries[0].buffer.minBindingSize = 0;
    layoutEntries[0].sampler.nextInChain = nullptr;
    layoutEntries[0].sampler.type = WGPUSamplerBindingType_Undefined;
    layoutEntries[0].texture.nextInChain = nullptr;
    layoutEntries[0].texture.sampleType = WGPUTextureSampleType_Undefined;
    layoutEntries[0].texture.viewDimension = WGPUTextureViewDimension_Undefined;
    layoutEntries[0].texture.multisampled = false;
    layoutEntries[0].storageTexture.nextInChain = nullptr;
    layoutEntries[0].storageTexture.access = WGPUStorageTextureAccess_Undefined;
    layoutEntries[0].storageTexture.format = WGPUTextureFormat_Undefined;
    layoutEntries[0].storageTexture.viewDimension = WGPUTextureViewDimension_Undefined;

    layoutEntries[1].nextInChain = nullptr;
    layoutEntries[1].binding = 1;
    layoutEntries[1].visibility = WGPUShaderStage_Fragment | WGPUShaderStage_Compute;
    layoutEntries[1].buffer.nextInChain = nullptr;
    layoutEntries[1].buffer.type = WGPUBufferBindingType_Undefined;
    layoutEntries[1].buffer.hasDynamicOffset = false;
    layoutEntries[1].buffer.minBindingSize = 0;
    layoutEntries[1].sampler.nextInChain = nullptr;
    layoutEntries[1].sampler.type = WGPUSamplerBindingType_Undefined;
    layoutEntries[1].texture.nextInChain = nullptr;
    layoutEntries[1].texture.sampleType = WGPUTextureSampleType_Undefined;
    layoutEntries[1].texture.viewDimension = WGPUTextureViewDimension_Undefined;
    layoutEntries[1].texture.multisampled = false;
    layoutEntries[1].storageTexture.nextInChain = nullptr;
    layoutEntries[1].storageTexture.access = WGPUStorageTextureAccess_ReadOnly;
    layoutEntries[1].storageTexture.format = WGPUTextureFormat_RGBA32Float;
    layoutEntries[1].storageTexture.viewDimension = WGPUTextureViewDimension_2D;

    layoutEntries[2].nextInChain = nullptr;
    layoutEntries[2].binding = 2;
    layoutEntries[2].visibility = WGPUShaderStage_Fragment | WGPUShaderStage_Compute;
    layoutEntries[2].buffer.nextInChain = nullptr;
    layoutEntries[2].buffer.type = WGPUBufferBindingType_Undefined;
    layoutEntries[2].buffer.hasDynamicOffset = false;
    layoutEntries[2].buffer.minBindingSize = 0;
    layoutEntries[2].sampler.nextInChain = nullptr;
    layoutEntries[2].sampler.type = WGPUSamplerBindingType_Filtering;
    layoutEntries[2].texture.nextInChain = nullptr;
    layoutEntries[2].texture.sampleType = WGPUTextureSampleType_Undefined;
    layoutEntries[2].texture.viewDimension = WGPUTextureViewDimension_Undefined;
    layoutEntries[2].texture.multisampled = false;
    layoutEntries[2].storageTexture.nextInChain = nullptr;
    layoutEntries[2].storageTexture.access = WGPUStorageTextureAccess_Undefined;
    layoutEntries[2].storageTexture.format = WGPUTextureFormat_Undefined;
    layoutEntries[2].storageTexture.viewDimension = WGPUTextureViewDimension_Undefined;

    layoutEntries[3].nextInChain = nullptr;
    layoutEntries[3].binding = 3;
    layoutEntries[3].visibility = WGPUShaderStage_Fragment | WGPUShaderStage_Compute;
    layoutEntries[3].buffer.nextInChain = nullptr;
    layoutEntries[3].buffer.type = WGPUBufferBindingType_Undefined;
    layoutEntries[3].buffer.hasDynamicOffset = false;
    layoutEntries[3].buffer.minBindingSize = 0;
    layoutEntries[3].sampler.nextInChain = nullptr;
    layoutEntries[3].sampler.type = WGPUSamplerBindingType_Undefined;
    layoutEntries[3].texture.nextInChain = nullptr;
    layoutEntries[3].texture.sampleType = WGPUTextureSampleType_Float;
    layoutEntries[3].texture.viewDimension = WGPUTextureViewDimension_2DArray;
    layoutEntries[3].texture.multisampled = false;
    layoutEntries[3].storageTexture.nextInChain = nullptr;
    layoutEntries[3].storageTexture.access = WGPUStorageTextureAccess_Undefined;
    layoutEntries[3].storageTexture.format = WGPUTextureFormat_Undefined;
    layoutEntries[3].storageTexture.viewDimension = WGPUTextureViewDimension_Undefined;

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor;
    bindGroupLayoutDescriptor.nextInChain = nullptr;
    bindGroupLayoutDescriptor.label = "materials";
    bindGroupLayoutDescriptor.entryCount = 4;
    bindGroupLayoutDescriptor.entries = layoutEntries;

    return wgpuDeviceCreateBindGroupLayout(device, &bindGroupLayoutDescriptor);
}

WGPUBindGroup createMaterialBindGroup(WGPUDevice device, WGPUBindGroupLayout bindGroupLayout, WGPUBuffer materialBuffer, WGPUSampler textureSampler,
    WGPUTextureView albedoTexture, WGPUTextureView environmentTexture)
{
    WGPUBindGroupEntry entries[4];

    entries[0].nextInChain = nullptr;
    entries[0].binding = 0;
    entries[0].buffer = materialBuffer;
    entries[0].offset = 0;
    entries[0].size = wgpuBufferGetSize(materialBuffer);
    entries[0].sampler = nullptr;
    entries[0].textureView = nullptr;

    entries[1].nextInChain = nullptr;
    entries[1].binding = 1;
    entries[1].buffer = nullptr;
    entries[1].offset = 0;
    entries[1].size = 0;
    entries[1].sampler = nullptr;
    entries[1].textureView = environmentTexture;

    entries[2].nextInChain = nullptr;
    entries[2].binding = 2;
    entries[2].buffer = nullptr;
    entries[2].offset = 0;
    entries[2].size = 0;
    entries[2].sampler = textureSampler;
    entries[2].textureView = nullptr;

    entries[3].nextInChain = nullptr;
    entries[3].binding = 3;
    entries[3].buffer = nullptr;
    entries[3].offset = 0;
    entries[3].size = 0;
    entries[3].sampler = nullptr;
    entries[3].textureView = albedoTexture;

    WGPUBindGroupDescriptor bindGroupDescriptor;
    bindGroupDescriptor.nextInChain = nullptr;
    bindGroupDescriptor.label = "materials";
    bindGroupDescriptor.layout = bindGroupLayout;
    bindGroupDescriptor.entryCount = 4;
    bindGroupDescriptor.entries = entries;

    return wgpuDeviceCreateBindGroup(device, &bindGroupDescriptor);
}
