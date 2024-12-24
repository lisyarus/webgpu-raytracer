#pragma once

#include <webgpu.h>

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>

struct FrameProfiler
{
    FrameProfiler(WGPUQuerySet querySet, WGPUCommandEncoder commandEncoder);

    bool timestamp(std::string name);

    std::vector<std::string> grabNames();

    WGPUCommandEncoder commandEncoder();

private:
    WGPUQuerySet querySet_;
    WGPUCommandEncoder commandEncoder_;
    std::vector<std::string> names_;
    std::size_t maxSize_;
};

struct Profiler
{
    Profiler(WGPUDevice device);
    ~Profiler();

    FrameProfiler beginFrame(WGPUCommandEncoder commandEncoder);
    void endFrame(FrameProfiler frameProfiler);
    void poll();

    void dump();

private:
    WGPUDevice device_;
    WGPUQuerySet querySet_;

    struct BufferPair
    {
        WGPUBuffer resolveBuffer;
        WGPUBuffer mapBuffer;

        void destroy();
    };

    std::mutex availableBuffersMutex_;

    std::vector<BufferPair> availableBuffers_;

    struct PendingData
    {
        BufferPair buffers;
        std::vector<std::string> names;
        Profiler * parent;
    };

    std::vector<std::unique_ptr<PendingData>> preparedBuffers_;

    std::mutex pendingBuffersMutex_;
    std::vector<std::unique_ptr<PendingData>> pendingBuffers_;

    struct ProfilingData
    {
        std::uint32_t count = 0;
        double totalTime = 0.0;
    };

    std::mutex profilingResultsMutex_;
    std::unordered_map<std::string, ProfilingData> profilingResults_;

    WGPUBuffer newResolveBuffer();
    WGPUBuffer newMapBuffer();
    BufferPair newBufferPair();
};
