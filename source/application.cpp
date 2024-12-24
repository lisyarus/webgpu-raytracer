#include <webgpu-raytracer/application.hpp>
#include <webgpu-raytracer/sdl_wgpu.h>

#include <wgpu.h>

#include <iostream>
#include <vector>
#include <chrono>

Application::Application()
{
    // Create SDL2 window

    SDL_Init(SDL_INIT_VIDEO);

    window_ = SDL_CreateWindow("WebGPU Demo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width_, height_, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN_DESKTOP);

    // Create WebGPU instance

    WGPUInstanceExtras instanceExtras;
    instanceExtras.chain.next = nullptr;
    instanceExtras.chain.sType = (WGPUSType)WGPUSType_InstanceExtras;

#if defined(__APPLE__)
    instanceExtras.backends = WGPUInstanceBackend_Metal;
#elif defined(_WIN32)
    instanceExtras.backends = WGPUInstanceBackend_DX12;
#else
    instanceExtras.backends = WGPUInstanceBackend_Vulkan;
#endif

    instanceExtras.flags = WGPUInstanceFlag_Default;
    instanceExtras.dx12ShaderCompiler = WGPUDx12Compiler_Undefined;
    instanceExtras.gles3MinorVersion = WGPUGles3MinorVersion_Automatic;
    instanceExtras.dxilPath = nullptr;
    instanceExtras.dxcPath = nullptr;

    WGPUInstanceDescriptor instanceDescriptor;
    instanceDescriptor.nextInChain = (WGPUChainedStruct *)&instanceExtras;
    WGPUInstance instance = wgpuCreateInstance(&instanceDescriptor);

    std::cout << "Instance: " << instance << std::endl;

    // Create WebGPU surface

    surface_ = SDL_WGPU_CreateSurface(instance, window_);

    std::cout << "Surface: " << surface_ << std::endl;

    // Request WebGPU adapter

    WGPURequestAdapterOptions requestAdapterOptions;
    requestAdapterOptions.nextInChain = nullptr;
    requestAdapterOptions.compatibleSurface = surface_;
    requestAdapterOptions.powerPreference = WGPUPowerPreference_HighPerformance;
    requestAdapterOptions.forceFallbackAdapter = false;
    requestAdapterOptions.backendType = WGPUBackendType_Undefined;

    WGPUAdapter adapter = nullptr;

    auto requestAdapterCallback = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, char const * message, void * userdata){
        if (message)
            std::cout << "Adapter callback message: " << message << std::endl;

        if (status == WGPURequestAdapterStatus_Success)
            *(WGPUAdapter *)(userdata) = adapter;
        else
            throw std::runtime_error(message);
    };

    wgpuInstanceRequestAdapter(instance, &requestAdapterOptions, requestAdapterCallback, &adapter);

    std::cout << "Adapter: " << adapter << std::endl;

    if (!adapter)
        throw std::runtime_error("Adapter not created");

    // List adapter features

    std::vector<WGPUFeatureName> features;
    features.resize(wgpuAdapterEnumerateFeatures(adapter, nullptr));
    wgpuAdapterEnumerateFeatures(adapter, features.data());

    std::cout << "Adapter features:" << std::endl;
    for (auto const & feature : features)
        std::cout << "    " << feature << std::endl;

    // List adapter limits

    WGPUSupportedLimits supportedLimits;
    supportedLimits.nextInChain = nullptr;
    wgpuAdapterGetLimits(adapter, &supportedLimits);

    std::cout << "Supported limits:" << std::endl;
    std::cout << "    maxTextureDimension1D: " << supportedLimits.limits.maxTextureDimension1D << std::endl;
    std::cout << "    maxTextureDimension2D: " << supportedLimits.limits.maxTextureDimension2D << std::endl;
    std::cout << "    maxTextureDimension3D: " << supportedLimits.limits.maxTextureDimension3D << std::endl;
    std::cout << "    maxTextureArrayLayers: " << supportedLimits.limits.maxTextureArrayLayers << std::endl;
    std::cout << "    maxBindGroups: " << supportedLimits.limits.maxBindGroups << std::endl;
    std::cout << "    maxBindGroupsPlusVertexBuffers: " << supportedLimits.limits.maxBindGroupsPlusVertexBuffers << std::endl;
    std::cout << "    maxBindingsPerBindGroup: " << supportedLimits.limits.maxBindingsPerBindGroup << std::endl;
    std::cout << "    maxDynamicUniformBuffersPerPipelineLayout: " << supportedLimits.limits.maxDynamicUniformBuffersPerPipelineLayout << std::endl;
    std::cout << "    maxDynamicStorageBuffersPerPipelineLayout: " << supportedLimits.limits.maxDynamicStorageBuffersPerPipelineLayout << std::endl;
    std::cout << "    maxSampledTexturesPerShaderStage: " << supportedLimits.limits.maxSampledTexturesPerShaderStage << std::endl;
    std::cout << "    maxSamplersPerShaderStage: " << supportedLimits.limits.maxSamplersPerShaderStage << std::endl;
    std::cout << "    maxStorageBuffersPerShaderStage: " << supportedLimits.limits.maxStorageBuffersPerShaderStage << std::endl;
    std::cout << "    maxStorageTexturesPerShaderStage: " << supportedLimits.limits.maxStorageTexturesPerShaderStage << std::endl;
    std::cout << "    maxUniformBuffersPerShaderStage: " << supportedLimits.limits.maxUniformBuffersPerShaderStage << std::endl;
    std::cout << "    maxUniformBufferBindingSize: " << supportedLimits.limits.maxUniformBufferBindingSize << std::endl;
    std::cout << "    maxStorageBufferBindingSize: " << supportedLimits.limits.maxStorageBufferBindingSize << std::endl;
    std::cout << "    minUniformBufferOffsetAlignment: " << supportedLimits.limits.minUniformBufferOffsetAlignment << std::endl;
    std::cout << "    minStorageBufferOffsetAlignment: " << supportedLimits.limits.minStorageBufferOffsetAlignment << std::endl;
    std::cout << "    maxVertexBuffers: " << supportedLimits.limits.maxVertexBuffers << std::endl;
    std::cout << "    maxBufferSize: " << supportedLimits.limits.maxBufferSize << std::endl;
    std::cout << "    maxVertexAttributes: " << supportedLimits.limits.maxVertexAttributes << std::endl;
    std::cout << "    maxVertexBufferArrayStride: " << supportedLimits.limits.maxVertexBufferArrayStride << std::endl;
    std::cout << "    maxInterStageShaderComponents: " << supportedLimits.limits.maxInterStageShaderComponents << std::endl;
    std::cout << "    maxInterStageShaderVariables: " << supportedLimits.limits.maxInterStageShaderVariables << std::endl;
    std::cout << "    maxColorAttachments: " << supportedLimits.limits.maxColorAttachments << std::endl;
    std::cout << "    maxColorAttachmentBytesPerSample: " << supportedLimits.limits.maxColorAttachmentBytesPerSample << std::endl;
    std::cout << "    maxComputeWorkgroupStorageSize: " << supportedLimits.limits.maxComputeWorkgroupStorageSize << std::endl;
    std::cout << "    maxComputeInvocationsPerWorkgroup: " << supportedLimits.limits.maxComputeInvocationsPerWorkgroup << std::endl;
    std::cout << "    maxComputeWorkgroupSizeX: " << supportedLimits.limits.maxComputeWorkgroupSizeX << std::endl;
    std::cout << "    maxComputeWorkgroupSizeY: " << supportedLimits.limits.maxComputeWorkgroupSizeY << std::endl;
    std::cout << "    maxComputeWorkgroupSizeZ: " << supportedLimits.limits.maxComputeWorkgroupSizeZ << std::endl;
    std::cout << "    maxComputeWorkgroupsPerDimension: " << supportedLimits.limits.maxComputeWorkgroupsPerDimension << std::endl;

    // Request WebGPU device

    WGPURequiredLimits requiredLimits;
    requiredLimits.nextInChain = nullptr;
    requiredLimits.limits = supportedLimits.limits;

    WGPUFeatureName requiredFeatures[3]
    {
        WGPUFeatureName_Float32Filterable,
        WGPUFeatureName_TimestampQuery,
        (WGPUFeatureName)WGPUNativeFeature_TextureAdapterSpecificFormatFeatures,
    };

    WGPUDeviceDescriptor deviceDescriptor;
    deviceDescriptor.nextInChain = nullptr;
    deviceDescriptor.label = nullptr;
    deviceDescriptor.requiredFeatureCount = 3;
    deviceDescriptor.requiredFeatures = requiredFeatures;
    deviceDescriptor.requiredLimits = &requiredLimits;
    deviceDescriptor.defaultQueue.nextInChain = nullptr;
    deviceDescriptor.defaultQueue.label = nullptr;
    deviceDescriptor.deviceLostCallback = nullptr;
    deviceDescriptor.deviceLostUserdata = nullptr;

    auto requestDeviceCallback = [](WGPURequestDeviceStatus status, WGPUDevice device, char const * message, void * userdata)
    {
        if (message)
            std::cout << "Device callback message: " << message << std::endl;

        if (status == WGPURequestDeviceStatus_Success)
            *(WGPUDevice *)(userdata) = device;
        else
            throw std::runtime_error(message);
    };

    wgpuAdapterRequestDevice(adapter, &deviceDescriptor, requestDeviceCallback, &device_);

    std::cout << "Device: " << device_ << std::endl;

    if (!device_)
        throw std::runtime_error("Device not created");

    // Get preferred format for this combination of surface + adapter

    surfaceFormat_ = WGPUTextureFormat_BGRA8UnormSrgb;

    WGPUSurfaceCapabilities surfaceCapabilities;
    wgpuSurfaceGetCapabilities(surface_, adapter, &surfaceCapabilities);

    auto formatsBegin = surfaceCapabilities.formats;
    auto formatsEnd = surfaceCapabilities.formats + surfaceCapabilities.formatCount;
    if (std::find(formatsBegin, formatsEnd, surfaceFormat_) == formatsEnd)
        throw std::runtime_error("Surface doesn't support BRGA8 sRGB format");

    std::cout << "Surface format: " << surfaceFormat_ << std::endl;
    std::cout << "Supported formats:\n";

    for (auto it = formatsBegin; it != formatsEnd; ++it)
        std::cout << "    " << *it << "\n";
    std::cout << std::flush;

    // Configure the surface to be presented

    resize(width_, height_, true);

    // Get the queue associated with the device

    queue_ = wgpuDeviceGetQueue(device_);

    // Adapter & instance are only needed during initialization - release them

    wgpuAdapterRelease(adapter);
    wgpuInstanceRelease(instance);
}

Application::~Application()
{
    wgpuQueueRelease(queue_);
    wgpuDeviceRelease(device_);
    wgpuSurfaceRelease(surface_);
    SDL_DestroyWindow(window_);
}

WGPUTexture Application::nextSwapchainTexture()
{
    // Request the current swapchain texture, measuring the time it took

    WGPUSurfaceTexture surfaceTexture;

    auto getTextureStart = std::chrono::high_resolution_clock::now();

    wgpuSurfaceGetCurrentTexture(surface_, &surfaceTexture);

    if (surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_Timeout)
    {
        // Don't treat it as an error
        // Feels suspicious though!
        std::cout << "Timeout" << std::endl;
        return nullptr;
    }

    auto getTextureEnd = std::chrono::high_resolution_clock::now();

    // N.B.: this time will be something like 1/FPS
//    std::cout << "Waited " << (std::chrono::duration_cast<std::chrono::duration<double>>(getTextureEnd - getTextureStart).count() * 1000.0) << " ms for the swapchain image" << std::endl;

    if (surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_Outdated)
        return nullptr;

    if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success)
        throw std::runtime_error("Can't get surface texture: " + std::to_string(surfaceTexture.status));

    return surfaceTexture.texture;
}

void Application::resize(int width, int height, bool vsync)
{
    width_ = width;
    height_ = height;

    // Reconfigure the surface for the new size

    WGPUSurfaceConfiguration surfaceConfiguration;
    surfaceConfiguration.nextInChain = nullptr;
    surfaceConfiguration.device = device_;
    surfaceConfiguration.format = surfaceFormat_;
    surfaceConfiguration.usage = WGPUTextureUsage_RenderAttachment;
    surfaceConfiguration.viewFormatCount = 1;
    surfaceConfiguration.viewFormats = &surfaceFormat_;
    surfaceConfiguration.alphaMode = WGPUCompositeAlphaMode_Auto;
    surfaceConfiguration.width = width_;
    surfaceConfiguration.height = height_;
    surfaceConfiguration.presentMode = vsync ? WGPUPresentMode_Fifo : WGPUPresentMode_Immediate;

    wgpuSurfaceConfigure(surface_, &surfaceConfiguration);

    std::cout << "Resized to " << width_ << "x" << height_ << std::endl;
}

void Application::present()
{
    wgpuSurfacePresent(surface_);
}

void Application::setMouseHidden(bool hidden)
{
    if (hidden)
        SDL_SetRelativeMouseMode(SDL_TRUE);
    else
        SDL_SetRelativeMouseMode(SDL_FALSE);
}

std::optional<SDL_Event> Application::poll()
{
    SDL_Event result;
    if (SDL_PollEvent(&result))
        return result;
    return std::nullopt;
}
