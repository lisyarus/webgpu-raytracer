#include <webgpu-raytracer/geometry_bind_group.hpp>

WGPUBindGroupLayout createGeometryBindGroupLayout(WGPUDevice device)
{
    WGPUBindGroupLayoutEntry layoutEntries[2];

    layoutEntries[0].nextInChain = nullptr;
    layoutEntries[0].binding = 0;
    layoutEntries[0].visibility = WGPUShaderStage_Fragment | WGPUShaderStage_Compute;
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
    layoutEntries[1].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
    layoutEntries[1].buffer.hasDynamicOffset = false;
    layoutEntries[1].buffer.minBindingSize = 0;
    layoutEntries[1].sampler.nextInChain = nullptr;
    layoutEntries[1].sampler.type = WGPUSamplerBindingType_Undefined;
    layoutEntries[1].texture.nextInChain = nullptr;
    layoutEntries[1].texture.sampleType = WGPUTextureSampleType_Undefined;
    layoutEntries[1].texture.viewDimension = WGPUTextureViewDimension_Undefined;
    layoutEntries[1].texture.multisampled = false;
    layoutEntries[1].storageTexture.nextInChain = nullptr;
    layoutEntries[1].storageTexture.access = WGPUStorageTextureAccess_Undefined;
    layoutEntries[1].storageTexture.format = WGPUTextureFormat_Undefined;
    layoutEntries[1].storageTexture.viewDimension = WGPUTextureViewDimension_Undefined;

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor;
    bindGroupLayoutDescriptor.nextInChain = nullptr;
    bindGroupLayoutDescriptor.label = "geometry";
    bindGroupLayoutDescriptor.entryCount = 2;
    bindGroupLayoutDescriptor.entries = layoutEntries;

    return wgpuDeviceCreateBindGroupLayout(device, &bindGroupLayoutDescriptor);
}

WGPUBindGroup createGeometryBindGroup(WGPUDevice device, WGPUBindGroupLayout bindGroupLayout, WGPUBuffer vertexBuffer, WGPUBuffer indexBuffer)
{
    WGPUBindGroupEntry entries[2];

    entries[0].nextInChain = nullptr;
    entries[0].binding = 0;
    entries[0].buffer = vertexBuffer;
    entries[0].offset = 0;
    entries[0].size = wgpuBufferGetSize(vertexBuffer);
    entries[0].sampler = nullptr;
    entries[0].textureView = nullptr;

    entries[1].nextInChain = nullptr;
    entries[1].binding = 1;
    entries[1].buffer = indexBuffer;
    entries[1].offset = 0;
    entries[1].size = wgpuBufferGetSize(indexBuffer);
    entries[1].sampler = nullptr;
    entries[1].textureView = nullptr;

    WGPUBindGroupDescriptor bindGroupDescriptor;
    bindGroupDescriptor.nextInChain = nullptr;
    bindGroupDescriptor.label = "geometry";
    bindGroupDescriptor.layout = bindGroupLayout;
    bindGroupDescriptor.entryCount = 2;
    bindGroupDescriptor.entries = entries;

    return wgpuDeviceCreateBindGroup(device, &bindGroupDescriptor);
}
