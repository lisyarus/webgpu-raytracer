#pragma once

#include <webgpu-raytracer/gltf_asset.hpp>

#include <filesystem>

namespace glTF
{

    Asset load(std::filesystem::path const & path);

    std::vector<char> loadBuffer(std::filesystem::path const & assetPath, std::string const & bufferUri);

    struct ImageInfo
    {
        std::uint32_t width;
        std::uint32_t height;
        std::vector<std::uint32_t> data;
    };

    ImageInfo loadImage(std::filesystem::path const & imagePath);
    ImageInfo loadImage(std::filesystem::path const & assetPath, std::string const & imageUri);

}
