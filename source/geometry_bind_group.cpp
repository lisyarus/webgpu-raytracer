#include <webgpu-raytracer/geometry_bind_group.hpp>

WGPUBindGroupLayout createGeometryBindGroupLayout(WGPUDevice device)
{
    WGPUBindGroupLayoutEntry layoutEntries[6];

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

    layoutEntries[2].nextInChain = nullptr;
    layoutEntries[2].binding = 2;
    layoutEntries[2].visibility = WGPUShaderStage_Fragment | WGPUShaderStage_Compute;
    layoutEntries[2].buffer.nextInChain = nullptr;
    layoutEntries[2].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
    layoutEntries[2].buffer.hasDynamicOffset = false;
    layoutEntries[2].buffer.minBindingSize = 0;
    layoutEntries[2].sampler.nextInChain = nullptr;
    layoutEntries[2].sampler.type = WGPUSamplerBindingType_Undefined;
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
    layoutEntries[3].visibility = WGPUShaderStage_Compute;
    layoutEntries[3].buffer.nextInChain = nullptr;
    layoutEntries[3].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
    layoutEntries[3].buffer.hasDynamicOffset = false;
    layoutEntries[3].buffer.minBindingSize = 0;
    layoutEntries[3].sampler.nextInChain = nullptr;
    layoutEntries[3].sampler.type = WGPUSamplerBindingType_Undefined;
    layoutEntries[3].texture.nextInChain = nullptr;
    layoutEntries[3].texture.sampleType = WGPUTextureSampleType_Undefined;
    layoutEntries[3].texture.viewDimension = WGPUTextureViewDimension_Undefined;
    layoutEntries[3].texture.multisampled = false;
    layoutEntries[3].storageTexture.nextInChain = nullptr;
    layoutEntries[3].storageTexture.access = WGPUStorageTextureAccess_Undefined;
    layoutEntries[3].storageTexture.format = WGPUTextureFormat_Undefined;
    layoutEntries[3].storageTexture.viewDimension = WGPUTextureViewDimension_Undefined;

    layoutEntries[4].nextInChain = nullptr;
    layoutEntries[4].binding = 4;
    layoutEntries[4].visibility = WGPUShaderStage_Compute;
    layoutEntries[4].buffer.nextInChain = nullptr;
    layoutEntries[4].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
    layoutEntries[4].buffer.hasDynamicOffset = false;
    layoutEntries[4].buffer.minBindingSize = 0;
    layoutEntries[4].sampler.nextInChain = nullptr;
    layoutEntries[4].sampler.type = WGPUSamplerBindingType_Undefined;
    layoutEntries[4].texture.nextInChain = nullptr;
    layoutEntries[4].texture.sampleType = WGPUTextureSampleType_Undefined;
    layoutEntries[4].texture.viewDimension = WGPUTextureViewDimension_Undefined;
    layoutEntries[4].texture.multisampled = false;
    layoutEntries[4].storageTexture.nextInChain = nullptr;
    layoutEntries[4].storageTexture.access = WGPUStorageTextureAccess_Undefined;
    layoutEntries[4].storageTexture.format = WGPUTextureFormat_Undefined;
    layoutEntries[4].storageTexture.viewDimension = WGPUTextureViewDimension_Undefined;

    layoutEntries[5].nextInChain = nullptr;
    layoutEntries[5].binding = 5;
    layoutEntries[5].visibility = WGPUShaderStage_Compute;
    layoutEntries[5].buffer.nextInChain = nullptr;
    layoutEntries[5].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
    layoutEntries[5].buffer.hasDynamicOffset = false;
    layoutEntries[5].buffer.minBindingSize = 0;
    layoutEntries[5].sampler.nextInChain = nullptr;
    layoutEntries[5].sampler.type = WGPUSamplerBindingType_Undefined;
    layoutEntries[5].texture.nextInChain = nullptr;
    layoutEntries[5].texture.sampleType = WGPUTextureSampleType_Undefined;
    layoutEntries[5].texture.viewDimension = WGPUTextureViewDimension_Undefined;
    layoutEntries[5].texture.multisampled = false;
    layoutEntries[5].storageTexture.nextInChain = nullptr;
    layoutEntries[5].storageTexture.access = WGPUStorageTextureAccess_Undefined;
    layoutEntries[5].storageTexture.format = WGPUTextureFormat_Undefined;
    layoutEntries[5].storageTexture.viewDimension = WGPUTextureViewDimension_Undefined;

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor;
    bindGroupLayoutDescriptor.nextInChain = nullptr;
    bindGroupLayoutDescriptor.label = "geometry";
    bindGroupLayoutDescriptor.entryCount = 6;
    bindGroupLayoutDescriptor.entries = layoutEntries;

    return wgpuDeviceCreateBindGroupLayout(device, &bindGroupLayoutDescriptor);
}

WGPUBindGroup createGeometryBindGroup(WGPUDevice device, WGPUBindGroupLayout bindGroupLayout, WGPUBuffer vertexPositionsBuffer,
    WGPUBuffer vertexAttributesBuffer,WGPUBuffer bvhNodesBuffer,
    WGPUBuffer emissiveTrianglesBuffer, WGPUBuffer emissiveTrianglesAliasBuffer, WGPUBuffer emissiveBvhNodesBuffer)
{
    WGPUBindGroupEntry entries[6];

    entries[0].nextInChain = nullptr;
    entries[0].binding = 0;
    entries[0].buffer = vertexPositionsBuffer;
    entries[0].offset = 0;
    entries[0].size = wgpuBufferGetSize(vertexPositionsBuffer);
    entries[0].sampler = nullptr;
    entries[0].textureView = nullptr;

    entries[1].nextInChain = nullptr;
    entries[1].binding = 1;
    entries[1].buffer = vertexAttributesBuffer;
    entries[1].offset = 0;
    entries[1].size = wgpuBufferGetSize(vertexAttributesBuffer);
    entries[1].sampler = nullptr;
    entries[1].textureView = nullptr;

    entries[2].nextInChain = nullptr;
    entries[2].binding = 2;
    entries[2].buffer = bvhNodesBuffer;
    entries[2].offset = 0;
    entries[2].size = wgpuBufferGetSize(bvhNodesBuffer);
    entries[2].sampler = nullptr;
    entries[2].textureView = nullptr;

    entries[3].nextInChain = nullptr;
    entries[3].binding = 3;
    entries[3].buffer = emissiveTrianglesBuffer;
    entries[3].offset = 0;
    entries[3].size = wgpuBufferGetSize(emissiveTrianglesBuffer);
    entries[3].sampler = nullptr;
    entries[3].textureView = nullptr;

    entries[4].nextInChain = nullptr;
    entries[4].binding = 4;
    entries[4].buffer = emissiveTrianglesAliasBuffer;
    entries[4].offset = 0;
    entries[4].size = wgpuBufferGetSize(emissiveTrianglesAliasBuffer);
    entries[4].sampler = nullptr;
    entries[4].textureView = nullptr;

    entries[5].nextInChain = nullptr;
    entries[5].binding = 5;
    entries[5].buffer = emissiveBvhNodesBuffer;
    entries[5].offset = 0;
    entries[5].size = wgpuBufferGetSize(emissiveBvhNodesBuffer);
    entries[5].sampler = nullptr;
    entries[5].textureView = nullptr;

    WGPUBindGroupDescriptor bindGroupDescriptor;
    bindGroupDescriptor.nextInChain = nullptr;
    bindGroupDescriptor.label = "geometry";
    bindGroupDescriptor.layout = bindGroupLayout;
    bindGroupDescriptor.entryCount = 6;
    bindGroupDescriptor.entries = entries;

    return wgpuDeviceCreateBindGroup(device, &bindGroupDescriptor);
}
