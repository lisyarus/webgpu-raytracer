#include <webgpu-raytracer/material_bind_group.hpp>

WGPUBindGroupLayout createMaterialBindGroupLayout(WGPUDevice device)
{
    WGPUBindGroupLayoutEntry layoutEntry;
    layoutEntry.nextInChain = nullptr;
    layoutEntry.binding = 0;
    layoutEntry.visibility = WGPUShaderStage_Vertex;
    layoutEntry.buffer.nextInChain = nullptr;
    layoutEntry.buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
    layoutEntry.buffer.hasDynamicOffset = false;
    layoutEntry.buffer.minBindingSize = 0;
    layoutEntry.sampler.nextInChain = nullptr;
    layoutEntry.sampler.type = WGPUSamplerBindingType_Undefined;
    layoutEntry.texture.nextInChain = nullptr;
    layoutEntry.texture.sampleType = WGPUTextureSampleType_Undefined;
    layoutEntry.texture.viewDimension = WGPUTextureViewDimension_Undefined;
    layoutEntry.texture.multisampled = false;
    layoutEntry.storageTexture.nextInChain = nullptr;
    layoutEntry.storageTexture.access = WGPUStorageTextureAccess_Undefined;
    layoutEntry.storageTexture.format = WGPUTextureFormat_Undefined;
    layoutEntry.storageTexture.viewDimension = WGPUTextureViewDimension_Undefined;

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor;
    bindGroupLayoutDescriptor.nextInChain = nullptr;
    bindGroupLayoutDescriptor.label = "materials";
    bindGroupLayoutDescriptor.entryCount = 1;
    bindGroupLayoutDescriptor.entries = &layoutEntry;

    return wgpuDeviceCreateBindGroupLayout(device, &bindGroupLayoutDescriptor);
}

WGPUBindGroup createMaterialBindGroup(WGPUDevice device, WGPUBindGroupLayout bindGroupLayout, WGPUBuffer materialBuffer)
{
    WGPUBindGroupEntry entry;
    entry.nextInChain = nullptr;
    entry.binding = 0;
    entry.buffer = materialBuffer;
    entry.offset = 0;
    entry.size = wgpuBufferGetSize(materialBuffer);
    entry.sampler = nullptr;
    entry.textureView = nullptr;

    WGPUBindGroupDescriptor bindGroupDescriptor;
    bindGroupDescriptor.nextInChain = nullptr;
    bindGroupDescriptor.label = "materials";
    bindGroupDescriptor.layout = bindGroupLayout;
    bindGroupDescriptor.entryCount = 1;
    bindGroupDescriptor.entries = &entry;

    return wgpuDeviceCreateBindGroup(device, &bindGroupDescriptor);
}
