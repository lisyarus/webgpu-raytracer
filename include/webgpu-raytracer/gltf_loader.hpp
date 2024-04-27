#pragma once

#include <webgpu-raytracer/gltf_asset.hpp>

#include <filesystem>

namespace glTF
{

    Asset load(std::filesystem::path const & path);

}
