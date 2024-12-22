#include <webgpu-raytracer/camera_bind_group.hpp>

namespace
{

    struct CameraUniform
    {
        glm::mat4 viewProjectionMatrix;
        glm::mat4 viewProjectionInverseMatrix;
        glm::vec3 position;
        char padding1[4];
        glm::uvec2 screenSize;
        std::uint32_t frameID;
        std::uint32_t globalFrameID;
    };

}

CameraBindGroup::CameraBindGroup(WGPUDevice device)
{
    WGPUBufferDescriptor uniformBufferDescriptor;
    uniformBufferDescriptor.nextInChain = nullptr;
    uniformBufferDescriptor.label = "camera";
    uniformBufferDescriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
    uniformBufferDescriptor.size = sizeof(CameraUniform);
    uniformBufferDescriptor.mappedAtCreation = false;

    uniformBuffer_ = wgpuDeviceCreateBuffer(device, &uniformBufferDescriptor);

    WGPUBindGroupLayoutEntry layoutEntry;
    layoutEntry.nextInChain = nullptr;
    layoutEntry.binding = 0;
    layoutEntry.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment | WGPUShaderStage_Compute;
    layoutEntry.buffer.nextInChain = nullptr;
    layoutEntry.buffer.type = WGPUBufferBindingType_Uniform;
    layoutEntry.buffer.hasDynamicOffset = false;
    layoutEntry.buffer.minBindingSize = sizeof(CameraUniform);
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
    bindGroupLayoutDescriptor.label = "camera";
    bindGroupLayoutDescriptor.entryCount = 1;
    bindGroupLayoutDescriptor.entries = &layoutEntry;

    bindGroupLayout_ = wgpuDeviceCreateBindGroupLayout(device, &bindGroupLayoutDescriptor);

    WGPUBindGroupEntry entry;
    entry.nextInChain = nullptr;
    entry.binding = 0;
    entry.buffer = uniformBuffer_;
    entry.offset = 0;
    entry.size = sizeof(CameraUniform);
    entry.sampler = nullptr;
    entry.textureView = nullptr;

    WGPUBindGroupDescriptor bindGroupDescriptor;
    bindGroupDescriptor.nextInChain = nullptr;
    bindGroupDescriptor.label = "camera";
    bindGroupDescriptor.layout = bindGroupLayout_;
    bindGroupDescriptor.entryCount = 1;
    bindGroupDescriptor.entries = &entry;

    bindGroup_ = wgpuDeviceCreateBindGroup(device, &bindGroupDescriptor);
}

CameraBindGroup::~CameraBindGroup()
{
    wgpuBindGroupRelease(bindGroup_);
    wgpuBindGroupLayoutRelease(bindGroupLayout_);
    wgpuBufferRelease(uniformBuffer_);
}

void CameraBindGroup::update(WGPUQueue queue, Camera const & camera, glm::uvec2 const & screenSize, std::uint32_t frameID, std::uint32_t globalFrameID)
{
    CameraUniform uniform
    {
        .viewProjectionMatrix = camera.viewProjectionMatrix(),
        .viewProjectionInverseMatrix = glm::inverse(uniform.viewProjectionMatrix),
        .position = camera.position(),
        .screenSize = screenSize,
        .frameID = frameID,
        .globalFrameID = globalFrameID,
    };

    wgpuQueueWriteBuffer(queue, uniformBuffer_, 0, &uniform, sizeof(uniform));
}
