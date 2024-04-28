#include <webgpu-raytracer/accumulation_bind_group.hpp>

WGPUBindGroupLayout createAccumulationSampleBindGroupLayout(WGPUDevice device)
{
    WGPUBindGroupLayoutEntry layoutEntry;
    layoutEntry.nextInChain = nullptr;
    layoutEntry.binding = 0;
    layoutEntry.visibility = WGPUShaderStage_Fragment;
    layoutEntry.buffer.nextInChain = nullptr;
    layoutEntry.buffer.type = WGPUBufferBindingType_Undefined;
    layoutEntry.buffer.hasDynamicOffset = false;
    layoutEntry.buffer.minBindingSize = 0;
    layoutEntry.sampler.nextInChain = nullptr;
    layoutEntry.sampler.type = WGPUSamplerBindingType_Undefined;
    layoutEntry.texture.nextInChain = nullptr;
    layoutEntry.texture.sampleType = WGPUTextureSampleType_Float;
    layoutEntry.texture.viewDimension = WGPUTextureViewDimension_2D;
    layoutEntry.texture.multisampled = false;
    layoutEntry.storageTexture.nextInChain = nullptr;
    layoutEntry.storageTexture.access = WGPUStorageTextureAccess_Undefined;
    layoutEntry.storageTexture.format = WGPUTextureFormat_Undefined;
    layoutEntry.storageTexture.viewDimension = WGPUTextureViewDimension_Undefined;

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor;
    bindGroupLayoutDescriptor.nextInChain = nullptr;
    bindGroupLayoutDescriptor.label = "accumulation_sample";
    bindGroupLayoutDescriptor.entryCount = 1;
    bindGroupLayoutDescriptor.entries = &layoutEntry;

    return wgpuDeviceCreateBindGroupLayout(device, &bindGroupLayoutDescriptor);
}

WGPUBindGroupLayout createAccumulationStorageBindGroupLayout(WGPUDevice device, WGPUTextureFormat textureFormat)
{
    WGPUBindGroupLayoutEntry layoutEntry;
    layoutEntry.nextInChain = nullptr;
    layoutEntry.binding = 0;
    layoutEntry.visibility = WGPUShaderStage_Compute;
    layoutEntry.buffer.nextInChain = nullptr;
    layoutEntry.buffer.type = WGPUBufferBindingType_Undefined;
    layoutEntry.buffer.hasDynamicOffset = false;
    layoutEntry.buffer.minBindingSize = 0;
    layoutEntry.sampler.nextInChain = nullptr;
    layoutEntry.sampler.type = WGPUSamplerBindingType_Undefined;
    layoutEntry.texture.nextInChain = nullptr;
    layoutEntry.texture.sampleType = WGPUTextureSampleType_Undefined;
    layoutEntry.texture.viewDimension = WGPUTextureViewDimension_Undefined;
    layoutEntry.texture.multisampled = false;
    layoutEntry.storageTexture.nextInChain = nullptr;
    layoutEntry.storageTexture.access = WGPUStorageTextureAccess_ReadWrite;
    layoutEntry.storageTexture.format = textureFormat;
    layoutEntry.storageTexture.viewDimension = WGPUTextureViewDimension_2D;

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor;
    bindGroupLayoutDescriptor.nextInChain = nullptr;
    bindGroupLayoutDescriptor.label = "accumulation_storage";
    bindGroupLayoutDescriptor.entryCount = 1;
    bindGroupLayoutDescriptor.entries = &layoutEntry;

    return wgpuDeviceCreateBindGroupLayout(device, &bindGroupLayoutDescriptor);
}

WGPUBindGroup createAccumulationSampleBindGroup(WGPUDevice device, WGPUBindGroupLayout bindGroupLayout, WGPUTextureView accumulationTextureView)
{
    WGPUBindGroupEntry entry;
    entry.nextInChain = nullptr;
    entry.binding = 0;
    entry.buffer = nullptr;
    entry.offset = 0;
    entry.size = 0;
    entry.sampler = nullptr;
    entry.textureView = accumulationTextureView;

    WGPUBindGroupDescriptor bindGroupDescriptor;
    bindGroupDescriptor.nextInChain = nullptr;
    bindGroupDescriptor.label = "accumulation_sample";
    bindGroupDescriptor.layout = bindGroupLayout;
    bindGroupDescriptor.entryCount = 1;
    bindGroupDescriptor.entries = &entry;

    return wgpuDeviceCreateBindGroup(device, &bindGroupDescriptor);
}

WGPUBindGroup createAccumulationStorageBindGroup(WGPUDevice device, WGPUBindGroupLayout bindGroupLayout, WGPUTextureView accumulationTextureView)
{
    WGPUBindGroupEntry entry;
    entry.nextInChain = nullptr;
    entry.binding = 0;
    entry.buffer = nullptr;
    entry.offset = 0;
    entry.size = 0;
    entry.sampler = nullptr;
    entry.textureView = accumulationTextureView;

    WGPUBindGroupDescriptor bindGroupDescriptor;
    bindGroupDescriptor.nextInChain = nullptr;
    bindGroupDescriptor.label = "accumulation_storage";
    bindGroupDescriptor.layout = bindGroupLayout;
    bindGroupDescriptor.entryCount = 1;
    bindGroupDescriptor.entries = &entry;

    return wgpuDeviceCreateBindGroup(device, &bindGroupDescriptor);
}
