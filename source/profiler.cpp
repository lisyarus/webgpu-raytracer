#include <webgpu-raytracer/profiler.hpp>

#include <iostream>

static std::uint32_t MAX_QUERY_COUNT = 16;
static std::uint32_t RESOLVE_BUFFER_SIZE = MAX_QUERY_COUNT * 8;

FrameProfiler::FrameProfiler(WGPUQuerySet querySet, WGPUCommandEncoder commandEncoder)
    : querySet_(querySet)
    , commandEncoder_(commandEncoder)
    , maxSize_(wgpuQuerySetGetCount(querySet_))
{}

bool FrameProfiler::timestamp(std::string name)
{
    if (names_.size() >= maxSize_)
        return false;

    wgpuCommandEncoderWriteTimestamp(commandEncoder_, querySet_, names_.size());
    names_.push_back(std::move(name));

    return true;
}

std::vector<std::string> FrameProfiler::grabNames()
{
    return std::move(names_);
}

WGPUCommandEncoder FrameProfiler::commandEncoder()
{
    return commandEncoder_;
}

Profiler::Profiler(WGPUDevice device)
    : device_(device)
{
    WGPUQuerySetDescriptor querySetDescriptor;
    querySetDescriptor.nextInChain = nullptr;
    querySetDescriptor.label = nullptr;
    querySetDescriptor.type = WGPUQueryType_Timestamp;
    querySetDescriptor.count = MAX_QUERY_COUNT;

    querySet_ = wgpuDeviceCreateQuerySet(device_, &querySetDescriptor);
}

Profiler::~Profiler()
{
    {
        std::lock_guard lock{pendingBuffersMutex_};
        for (auto const & data : pendingBuffers_)
            data->buffers.destroy();
    }

    for (auto const & data : preparedBuffers_)
        data->buffers.destroy();

    {
        std::lock_guard lock{availableBuffersMutex_};
        for (auto & data : availableBuffers_)
            data.destroy();
    }

    wgpuQuerySetRelease(querySet_);
}

FrameProfiler Profiler::beginFrame(WGPUCommandEncoder commandEncoder)
{
    FrameProfiler frameProfiler(querySet_, commandEncoder);
    frameProfiler.timestamp("beginFrame");
    return frameProfiler;
}

void Profiler::endFrame(FrameProfiler frameProfiler)
{
    auto data = std::make_unique<PendingData>();

    if (availableBuffers_.empty())
        data->buffers = newBufferPair();
    else
    {
        data->buffers = availableBuffers_.back();
        availableBuffers_.pop_back();
    }

    data->names = frameProfiler.grabNames();

    data->parent = this;

    wgpuCommandEncoderResolveQuerySet(frameProfiler.commandEncoder(), querySet_, 0, data->names.size(), data->buffers.resolveBuffer, 0);
    wgpuCommandEncoderCopyBufferToBuffer(frameProfiler.commandEncoder(), data->buffers.resolveBuffer, 0, data->buffers.mapBuffer, 0, RESOLVE_BUFFER_SIZE);

    preparedBuffers_.push_back(std::move(data));
}

void Profiler::poll()
{
    for (auto & data : preparedBuffers_)
    {
        pendingBuffers_.push_back(std::move(data));
        auto buffers = pendingBuffers_.back()->buffers;

        auto callback = [](WGPUBufferMapAsyncStatus status, void * userData)
        {
            if (status != WGPUBufferMapAsyncStatus_Success)
                return;

            auto data = (PendingData *)userData;
            auto buffers = data->buffers;

            auto values = (std::uint64_t const *)wgpuBufferGetConstMappedRange(buffers.mapBuffer, 0, RESOLVE_BUFFER_SIZE);

            for (int i = 1; i < data->names.size(); ++i)
            {
                double deltaTime = (values[i] - values[i - 1]) / 1e9;

                std::lock_guard lock{data->parent->profilingResultsMutex_};
                auto & result = data->parent->profilingResults_[data->names[i]];
                result.count += 1;
                result.totalTime += deltaTime;
            }

            wgpuBufferUnmap(buffers.mapBuffer);

            {
                std::lock_guard lock{data->parent->pendingBuffersMutex_};
                for (auto it = data->parent->pendingBuffers_.begin(); it != data->parent->pendingBuffers_.end(); ++it)
                {
                    if (it->get() == data)
                    {
                        data->parent->pendingBuffers_.erase(it);
                        break;
                    }
                }
            }

            {
                std::lock_guard lock{data->parent->availableBuffersMutex_};
                data->parent->availableBuffers_.push_back(buffers);
            }
        };

        wgpuBufferMapAsync(buffers.mapBuffer, WGPUMapMode_Read, 0, RESOLVE_BUFFER_SIZE, callback, pendingBuffers_.back().get());
    }

    preparedBuffers_.clear();
}

void Profiler::dump()
{
    {
        std::lock_guard lock{profilingResultsMutex_};
        for (auto const & result : profilingResults_)
            std::cout << result.first << "        " << (result.second.totalTime / result.second.count * 1000.0) << " ms (x" << result.second.count << ")\n";
    }
    std::cout << std::flush;
}

void Profiler::BufferPair::destroy()
{
    wgpuBufferRelease(mapBuffer);
    wgpuBufferRelease(resolveBuffer);
}

WGPUBuffer Profiler::newResolveBuffer()
{
    WGPUBufferDescriptor bufferDescriptor;
    bufferDescriptor.nextInChain = nullptr;
    bufferDescriptor.label = nullptr;
    bufferDescriptor.usage = WGPUBufferUsage_QueryResolve | WGPUBufferUsage_CopySrc;
    bufferDescriptor.size = RESOLVE_BUFFER_SIZE;
    bufferDescriptor.mappedAtCreation = false;

    return wgpuDeviceCreateBuffer(device_, &bufferDescriptor);
}

WGPUBuffer Profiler::newMapBuffer()
{
    WGPUBufferDescriptor bufferDescriptor;
    bufferDescriptor.nextInChain = nullptr;
    bufferDescriptor.label = nullptr;
    bufferDescriptor.usage = WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst;
    bufferDescriptor.size = RESOLVE_BUFFER_SIZE;
    bufferDescriptor.mappedAtCreation = false;

    return wgpuDeviceCreateBuffer(device_, &bufferDescriptor);
}

Profiler::BufferPair Profiler::newBufferPair()
{
    return {
        .resolveBuffer = newResolveBuffer(),
        .mapBuffer = newMapBuffer(),
    };
}
