#include <webgpu-raytracer/shader_registry.hpp>

#include <fstream>
#include <unordered_map>
#include <unordered_set>

struct ShaderRegistry::Impl
{
    Impl(std::filesystem::path const & shadersPath, WGPUDevice device);
    ~Impl();

    WGPUShaderModule loadShaderModule(std::string const & name);

private:
    std::filesystem::path shadersPath_;
    WGPUDevice device_;
    std::unordered_map<std::string, WGPUShaderModule> cachedShaderModules_;

    struct LoadingContext
    {
        std::unordered_set<std::string> alreadyIncluded;
    };

    std::string loadSource(std::string const & name, LoadingContext & context);
};

ShaderRegistry::Impl::Impl(std::filesystem::path const & shadersPath, WGPUDevice device)
    : shadersPath_(shadersPath)
    , device_(device)
{}

ShaderRegistry::Impl::~Impl()
{
    for (auto const & shaderModule : cachedShaderModules_)
        wgpuShaderModuleRelease(shaderModule.second);
}

WGPUShaderModule ShaderRegistry::Impl::loadShaderModule(std::string const & name)
{
    if (auto it = cachedShaderModules_.find(name); it != cachedShaderModules_.end())
        return it->second;

    LoadingContext context;
    auto const & source = loadSource(name + ".wgsl", context);

    WGPUShaderModuleWGSLDescriptor wgslDescriptor;
    wgslDescriptor.chain.next = nullptr;
    wgslDescriptor.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
    wgslDescriptor.code = source.data();

    WGPUShaderModuleDescriptor shaderModuleDescriptor;
    shaderModuleDescriptor.nextInChain = (WGPUChainedStruct *)&wgslDescriptor;
    shaderModuleDescriptor.label = name.data();
    shaderModuleDescriptor.hintCount = 0;
    shaderModuleDescriptor.hints = nullptr;

    WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(device_, &shaderModuleDescriptor);
    cachedShaderModules_[name] = shaderModule;
    return shaderModule;
}

std::string ShaderRegistry::Impl::loadSource(std::string const & name, LoadingContext & context)
{
    auto path = shadersPath_ / name;

    std::string source(std::filesystem::file_size(path), '\0');

    {
        std::ifstream ifs(path);
        if (!ifs)
            throw std::runtime_error("Failed to load shader " + name);

        ifs.read(source.data(), source.size());
    }

    // Replace `use X.wgsl` statements with the source of X.wgsl

    for (std::size_t useStart = 0; (useStart = source.find("use ", useStart)) != std::string::npos;)
    {
        if (useStart == 0 || source[useStart - 1] == '\n')
        {
            auto useEnd = source.find(';', useStart + 4);
            auto useName = source.substr(useStart + 4, useEnd - (useStart + 4));

            useEnd = source.find('\n', useEnd) + 1;
            std::string useSource;
            if (context.alreadyIncluded.contains(useName))
                useSource = "\n";
            else
                useSource = loadSource(useName, context);
            source.replace(useStart, useEnd - useStart, useSource);
            useStart = useStart + useSource.size();

            context.alreadyIncluded.insert(useName);
        }
        else
        {
            useStart += 4;
        }
    }

    return source;
}

ShaderRegistry::ShaderRegistry(std::filesystem::path const & shadersPath, WGPUDevice device)
    : pimpl_(std::make_unique<Impl>(shadersPath, device))
{}

// Need to explicitly implement the destructor for pimpl to work
// See https://www.fluentcpp.com/2017/09/22/make-pimpl-using-unique_ptr
ShaderRegistry::~ShaderRegistry() = default;

WGPUShaderModule ShaderRegistry::loadShaderModule(std::string const & name)
{
    return pimpl_->loadShaderModule(name);
}
