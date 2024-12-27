#include <webgpu-raytracer/compose_uniforms_bind_group.hpp>

namespace
{

    struct ComposeUniforms
    {
        float exposure;
    };

}

ComposeUniformsBindGroup::ComposeUniformsBindGroup(WGPUDevice device)
{

    WGPUBufferDescriptor uniformBufferDescriptor;
    uniformBufferDescriptor.nextInChain = nullptr;
    uniformBufferDescriptor.label = "compose";
    uniformBufferDescriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
    uniformBufferDescriptor.size = sizeof(ComposeUniforms);
    uniformBufferDescriptor.mappedAtCreation = false;

    uniformBuffer_ = wgpuDeviceCreateBuffer(device, &uniformBufferDescriptor);

    WGPUBindGroupLayoutEntry layoutEntry;
    layoutEntry.nextInChain = nullptr;
    layoutEntry.binding = 0;
    layoutEntry.visibility = WGPUShaderStage_Fragment | WGPUShaderStage_Compute;
    layoutEntry.buffer.nextInChain = nullptr;
    layoutEntry.buffer.type = WGPUBufferBindingType_Uniform;
    layoutEntry.buffer.hasDynamicOffset = false;
    layoutEntry.buffer.minBindingSize = sizeof(ComposeUniforms);
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
    bindGroupLayoutDescriptor.label = "compose";
    bindGroupLayoutDescriptor.entryCount = 1;
    bindGroupLayoutDescriptor.entries = &layoutEntry;

    bindGroupLayout_ = wgpuDeviceCreateBindGroupLayout(device, &bindGroupLayoutDescriptor);

    WGPUBindGroupEntry entry;
    entry.nextInChain = nullptr;
    entry.binding = 0;
    entry.buffer = uniformBuffer_;
    entry.offset = 0;
    entry.size = sizeof(ComposeUniforms);
    entry.sampler = nullptr;
    entry.textureView = nullptr;

    WGPUBindGroupDescriptor bindGroupDescriptor;
    bindGroupDescriptor.nextInChain = nullptr;
    bindGroupDescriptor.label = "compose";
    bindGroupDescriptor.layout = bindGroupLayout_;
    bindGroupDescriptor.entryCount = 1;
    bindGroupDescriptor.entries = &entry;

    bindGroup_ = wgpuDeviceCreateBindGroup(device, &bindGroupDescriptor);
}

ComposeUniformsBindGroup::~ComposeUniformsBindGroup()
{
    wgpuBindGroupRelease(bindGroup_);
    wgpuBindGroupLayoutRelease(bindGroupLayout_);
}

void ComposeUniformsBindGroup::update(WGPUQueue queue, float exposure)
{
    ComposeUniforms uniforms
    {
        .exposure = exposure,
    };

    wgpuQueueWriteBuffer(queue, uniformBuffer_, 0, &uniforms, sizeof(uniforms));
}

