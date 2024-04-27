#pragma once

#include <webgpu.h>

#include <memory>
#include <string>
#include <filesystem>

struct ShaderRegistry
{
    ShaderRegistry(std::filesystem::path const & shadersPath, WGPUDevice device);
    ~ShaderRegistry();

    WGPUShaderModule loadShaderModule(std::string const & name);

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};
