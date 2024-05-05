#include <webgpu-raytracer/gltf_loader.hpp>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <fstream>
#include <stdexcept>
#include <iostream>

namespace glTF
{

    namespace
    {

        std::vector<char> loadBuffer(std::filesystem::path const & assetPath, std::string const & bufferUri)
        {
            auto const bufferPath = assetPath.parent_path() / bufferUri;

            std::vector<char> result;
            result.resize(std::filesystem::file_size(bufferPath));

            std::ifstream input(bufferPath, std::ios::binary);
            input.read(result.data(), result.size());

            return result;
        }

        Image loadImage(std::filesystem::path const & assetPath, std::string const & uri)
        {
            auto imagePath = assetPath.parent_path() / uri;

            Image result;
            result.uri = uri;

            int channels;
            auto pixels = stbi_load(imagePath.c_str(), (int *)&result.width, (int *)&result.height, &channels, 4);

            result.data.resize(result.width * result.height);
            std::copy(pixels, pixels + result.width * result.height * 4, (char *)result.data.data());

            stbi_image_free(pixels);

            return result;
        }

    }

    Asset load(std::filesystem::path const & path)
    {
        std::ifstream input(path);
        if (!input)
            throw std::runtime_error("Failed to open " + path.string());

        rapidjson::IStreamWrapper inputWrapper(input);
        rapidjson::Document document;
        document.ParseStream(inputWrapper);

        if (document.HasParseError())
            throw std::runtime_error("Failed to parse " + path.string() + ": " + std::to_string((int)document.GetParseError()));

        Asset result;

        if (document.HasMember("nodes"))
        for (auto const & nodeIn : document["nodes"].GetArray())
        {
            auto & node = result.nodes.emplace_back();

            if (nodeIn.HasMember("name"))
                node.name = nodeIn["name"].GetString();

            if (nodeIn.HasMember("children"))
                for (auto const & childId : nodeIn["children"].GetArray())
                    node.children.push_back(childId.GetUint());

            if (nodeIn.HasMember("mesh"))
                node.mesh = nodeIn["mesh"].GetUint();

            if (nodeIn.HasMember("camera"))
                node.camera = nodeIn["camera"].GetUint();

            if (nodeIn.HasMember("matrix"))
            {
                auto const & matrixIn = nodeIn["matrix"].GetArray();
                auto * matrixPtr = (float *)&node.matrix;
                for (int i = 0; i < 16; ++i)
                    matrixPtr[i] = matrixIn[i].GetFloat();
            }
            else
            {
                glm::vec3 translation{0.f};
                glm::quat rotation{1.f, 0.f, 0.f, 0.f};
                glm::vec3 scale{1.f};

                if (nodeIn.HasMember("translation"))
                {
                    auto const & translationIn = nodeIn["translation"].GetArray();
                    for (int i = 0; i < 3; ++i)
                        translation[i] = translationIn[i].GetFloat();
                }

                if (nodeIn.HasMember("rotation"))
                {
                    auto const & rotationIn = nodeIn["rotation"].GetArray();
                    for (int i = 0; i < 4; ++i)
                        rotation[i] = rotationIn[i].GetFloat();
                }

                if (nodeIn.HasMember("scale"))
                {
                    auto const & scaleIn = nodeIn["scale"].GetArray();
                    for (int i = 0; i < 3; ++i)
                        scale[i] = scaleIn[i].GetFloat();
                }

                node.matrix = glm::translate(glm::mat4(1.f), translation)
                    * glm::toMat4(rotation)
                    * glm::scale(glm::mat4(1.f), scale);
            }
        }

        if (document.HasMember("meshes"))
        for (auto const & meshIn : document["meshes"].GetArray())
        {
            auto & mesh = result.meshes.emplace_back();

            for (auto const & primitiveIn : meshIn["primitives"].GetArray())
            {
                auto & primitive = mesh.primitives.emplace_back();

                primitive.mode = Primitive::Mode::Triangles;
                if (primitiveIn.HasMember("mode"))
                    primitive.mode = static_cast<Primitive::Mode>(primitiveIn["mode"].GetUint());

                if (primitiveIn.HasMember("indices"))
                    primitive.indices = primitiveIn["indices"].GetUint();

                if (primitiveIn.HasMember("material"))
                    primitive.material = primitiveIn["material"].GetUint();

                auto const & attributes = primitiveIn["attributes"];

                if (attributes.HasMember("POSITION"))
                    primitive.attributes.position = attributes["POSITION"].GetUint();

                if (attributes.HasMember("NORMAL"))
                    primitive.attributes.normal = attributes["NORMAL"].GetUint();

                if (attributes.HasMember("TANGENT"))
                    primitive.attributes.tangent = attributes["TANGENT"].GetUint();

                if (attributes.HasMember("TEXCOORD_0"))
                    primitive.attributes.texcoord = attributes["TEXCOORD_0"].GetUint();
            }
        }

        if (document.HasMember("materials"))
        for (auto const & materialIn : document["materials"].GetArray())
        {
            auto & material = result.materials.emplace_back();

            material.baseColorFactor = glm::vec4(1.f);
            material.metallicFactor = 1.f;
            material.roughnessFactor = 1.f;
            material.emissiveFactor = glm::vec3(0.f);

            if (materialIn.HasMember("pbrMetallicRoughness"))
            {
                auto const & pbrMetallicRoughness = materialIn["pbrMetallicRoughness"];

                if (pbrMetallicRoughness.HasMember("baseColorFactor"))
                {
                    auto const & baseColorFactor = pbrMetallicRoughness["baseColorFactor"].GetArray();
                    for (int i = 0; i < 4; ++i)
                        material.baseColorFactor[i] = baseColorFactor[i].GetFloat();
                }

                if (pbrMetallicRoughness.HasMember("metallicFactor"))
                    material.metallicFactor = pbrMetallicRoughness["metallicFactor"].GetFloat();

                if (pbrMetallicRoughness.HasMember("roughnessFactor"))
                    material.roughnessFactor = pbrMetallicRoughness["roughnessFactor"].GetFloat();

                if (pbrMetallicRoughness.HasMember("baseColorTexture"))
                    material.baseColorTexture = pbrMetallicRoughness["baseColorTexture"]["index"].GetUint();

                if (pbrMetallicRoughness.HasMember("metallicRoughnessTexture"))
                    material.metallicRoughnessTexture = pbrMetallicRoughness["metallicRoughnessTexture"]["index"].GetUint();
            }

            if (materialIn.HasMember("emissiveFactor"))
            {
                auto const & emissiveFactor = materialIn["emissiveFactor"].GetArray();
                for (int i = 0; i < 3; ++i)
                    material.emissiveFactor[i] = emissiveFactor[i].GetFloat();
            }

            if (materialIn.HasMember("normalTexture"))
                material.normalTexture = materialIn["normalTexture"]["index"].GetUint();

            if (materialIn.HasMember("emissiveTexture"))
                material.emissiveTexture = materialIn["emissiveTexture"]["index"].GetUint();

            material.ior = 1.5f;

            if (materialIn.HasMember("extensions"))
            {
                auto const & extensions = materialIn["extensions"];

                if (extensions.HasMember("KHR_materials_emissive_strength"))
                {
                    material.emissiveFactor *= extensions["KHR_materials_emissive_strength"]["emissiveStrength"].GetFloat();
                }

                if (extensions.HasMember("KHR_materials_ior"))
                {
                    material.ior = extensions["KHR_materials_ior"]["ior"].GetFloat();
                }
            }
        }

        if (document.HasMember("images"))
        for (auto const & imageIn : document["images"].GetArray())
        {
            auto & image = result.images.emplace_back();
            image = loadImage(path, imageIn["uri"].GetString());
        }

        if (document.HasMember("textures"))
        for (auto const & textureIn : document["textures"].GetArray())
        {
            auto & texture = result.textures.emplace_back();
            if (textureIn.HasMember("source"))
                texture.source = textureIn["source"].GetUint();
        }

        if (document.HasMember("accessors"))
        for (auto const & accessorIn : document["accessors"].GetArray())
        {
            auto & accessor = result.accessors.emplace_back();

            accessor.bufferView = accessorIn["bufferView"].GetUint();

            accessor.byteOffset = 0;
            if (accessorIn.HasMember("byteOffset"))
                accessor.byteOffset = accessorIn["byteOffset"].GetUint();

            accessor.componentType = static_cast<Accessor::ComponentType>(accessorIn["componentType"].GetUint());

            accessor.normalized = false;
            if (accessorIn.HasMember("normalized"))
                accessor.normalized = accessorIn["normalized"].GetBool();

            accessor.count = accessorIn["count"].GetUint();

            std::string const typeStr = accessorIn["type"].GetString();
            if (typeStr == "SCALAR")
                accessor.type = Accessor::Type::Scalar;
            else if (typeStr == "VEC2")
                accessor.type = Accessor::Type::Vec2;
            else if (typeStr == "VEC3")
                accessor.type = Accessor::Type::Vec3;
            else if (typeStr == "VEC4")
                accessor.type = Accessor::Type::Vec4;
            else if (typeStr == "MAT2")
                accessor.type = Accessor::Type::Mat2;
            else if (typeStr == "MAT3")
                accessor.type = Accessor::Type::Mat3;
            else if (typeStr == "MAT4")
                accessor.type = Accessor::Type::Mat4;
            else
                accessor.type = Accessor::Type::Unknown;
        }

        if (document.HasMember("bufferViews"))
        for (auto const & bufferViewIn : document["bufferViews"].GetArray())
        {
            auto & bufferView = result.bufferViews.emplace_back();

            bufferView.buffer = bufferViewIn["buffer"].GetUint();

            bufferView.byteOffset = 0;
            if (bufferViewIn.HasMember("byteOffset"))
                bufferView.byteOffset = bufferViewIn["byteOffset"].GetUint();

            bufferView.byteLength = bufferViewIn["byteLength"].GetUint();

            if (bufferViewIn.HasMember("byteStride"))
                bufferView.byteStride = bufferViewIn["byteStride"].GetUint();
        }

        if (document.HasMember("buffers"))
        for (auto const & bufferIn : document["buffers"].GetArray())
        {
            auto & buffer = result.buffers.emplace_back();

            buffer.uri = bufferIn["uri"].GetString();
            buffer.data = loadBuffer(path, buffer.uri);
        }

        if (document.HasMember("cameras"))
        for (auto const & cameraIn : document["cameras"].GetArray())
        {
            auto & camera = result.cameras.emplace_back();

            if (cameraIn.HasMember("perspective"))
            {
                auto & perspective = cameraIn["perspective"];
                camera.yFov = perspective["yfov"].GetFloat();
                camera.zNear = perspective["znear"].GetFloat();
                if (perspective.HasMember("zfar"))
                    camera.zFar = perspective["zfar"].GetFloat();
            }
            else
            {
                std::cout << "Warning: non-perspective cameras are not supported\n";
                camera.yFov = glm::radians(90.f);
                camera.zNear = 0.01f;
            }
        }

        return result;
    }

}
