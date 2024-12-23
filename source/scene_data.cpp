#include <webgpu-raytracer/scene_data.hpp>
#include <webgpu-raytracer/gltf_iterator.hpp>
#include <webgpu-raytracer/material_bind_group.hpp>
#include <webgpu-raytracer/geometry_bind_group.hpp>
#include <webgpu-raytracer/bvh.hpp>
#include <stb_image.h>

#include <glm/glm.hpp>

#include <iostream>

namespace
{

    struct VertexAttributes
    {
        glm::vec3 normal;
        std::uint32_t materialID;
        glm::vec2 texcoords;
        char padding[8];
    };

    static_assert(sizeof(VertexAttributes) == 32);

    struct Vertex
    {
        glm::vec3 position;
        std::uint32_t padding = 0;
        VertexAttributes attributes;
    };

    struct Material
    {
        glm::vec4 baseColorFactorAndTransmission;
        // vec4(0, roughness, metallic, ior)
        glm::vec4 metallicRoughnessFactorAndIor;
        glm::vec4 emissiveFactor;
        // uvec4(albedo, 0, 0, 0)
        glm::uvec4 textureLayers;
    };

    void readIndices(glTF::Asset const & asset, glTF::Accessor const & indexAccessor, std::vector<std::uint32_t> & indices, std::uint32_t baseVertex)
    {
        switch (indexAccessor.componentType)
        {
        case glTF::Accessor::ComponentType::UnsignedByte:
            for (auto index : glTF::AccessorRange<std::uint8_t>(asset, indexAccessor))
                indices.push_back(baseVertex + index);
            break;
        case glTF::Accessor::ComponentType::UnsignedShort:
            for (auto index : glTF::AccessorRange<std::uint16_t>(asset, indexAccessor))
                indices.push_back(baseVertex + index);
            break;
        case glTF::Accessor::ComponentType::UnsignedInt:
            for (auto index : glTF::AccessorRange<std::uint32_t>(asset, indexAccessor))
                indices.push_back(baseVertex + index);
            break;
        default:
            std::cout << "Warning: unsupported index component type: " << (int)indexAccessor.componentType << "\n";
            break;
        }
    }

    void fillIndices(glTF::Accessor const & positionAccessor, std::vector<std::uint32_t> & indices, std::uint32_t baseVertex)
    {
        for (std::uint32_t index = 0; index < positionAccessor.count; ++index)
            indices.push_back(baseVertex + index);
    }

    void readPositions(glTF::Asset const & asset, glTF::Accessor const & positionAccessor, std::vector<Vertex> & vertices, std::uint32_t baseVertex)
    {
        auto vertexIt = vertices.begin() + baseVertex;

        switch (positionAccessor.componentType)
        {
        case glTF::Accessor::ComponentType::Float:
            for (auto position : glTF::AccessorRange<glm::vec3>(asset, positionAccessor))
            {
                vertexIt->position = position;
                ++vertexIt;
            }
            break;
        default:
            std::cout << "Warning: unsupported position component type: " << (int)positionAccessor.componentType << "\n";

            // Prevent uninitialized data
            for (std::uint32_t i = 0; i < positionAccessor.count; ++i)
            {
                vertexIt->position = glm::vec3(0.f);
                ++vertexIt;
            }
            break;
        }
    }

    void readNormals(glTF::Asset const & asset, glTF::Accessor const & normalAccessor, std::vector<Vertex> & vertices, std::uint32_t baseVertex)
    {
        auto vertexIt = vertices.begin() + baseVertex;

        switch (normalAccessor.componentType)
        {
        case glTF::Accessor::ComponentType::Float:
            for (auto normal : glTF::AccessorRange<glm::vec3>(asset, normalAccessor))
            {
                vertexIt->attributes.normal = normal;
                ++vertexIt;
            }
            break;
        default:
            std::cout << "Warning: unsupported normal component type: " << (int)normalAccessor.componentType << "\n";

            // Prevent uninitialized data
            for (std::uint32_t i = 0; i < normalAccessor.count; ++i)
            {
                vertexIt->attributes.normal = glm::vec3(0.f, 0.f, 1.f);
                ++vertexIt;
            }
            break;
        }
    }

    void reconstructNormals(std::vector<Vertex> & vertices, std::vector<std::uint32_t> const & indices, std::uint32_t baseVertex, std::uint32_t baseIndex,
        std::uint32_t vertexCount, std::uint32_t indexCount)
    {
        for (std::uint32_t i = 0; i < vertexCount; ++i)
            vertices[baseVertex + i].attributes.normal = glm::vec3(0.f);

        // Compute average adjacent triangle normal
        for (std::uint32_t i = 0; i < indexCount; i += 3)
        {
            auto & v0 = vertices[baseVertex + indices[baseIndex + i + 0]];
            auto & v1 = vertices[baseVertex + indices[baseIndex + i + 1]];
            auto & v2 = vertices[baseVertex + indices[baseIndex + i + 2]];

            auto normal = glm::normalize(glm::cross(v1.position - v0.position, v2.position - v0.position));

            v0.attributes.normal += normal;
            v1.attributes.normal += normal;
            v2.attributes.normal += normal;
        }

        for (std::uint32_t i = 0; i < vertexCount; ++i)
        {
            auto & v = vertices[baseVertex + i];
            v.attributes.normal = glm::normalize(v.attributes.normal);
        }
    }

    void fillDefaultTexcoords(std::vector<Vertex> & vertices, std::uint32_t baseVertex, std::uint32_t count)
    {
        auto vertexIt = vertices.begin() + baseVertex;
        for (std::uint32_t i = 0; i < count; ++i)
        {
            vertexIt->attributes.texcoords = {0.5f, 0.5f};
            ++vertexIt;
        }
    }

    void readTexcoords(glTF::Asset const & asset, glTF::Accessor const & texcoordAccessor, std::vector<Vertex> & vertices, std::uint32_t baseVertex)
    {
        auto vertexIt = vertices.begin() + baseVertex;

        switch (texcoordAccessor.componentType)
        {
        case glTF::Accessor::ComponentType::Float:
            for (auto texcoord : glTF::AccessorRange<glm::vec2>(asset, texcoordAccessor))
            {
                vertexIt->attributes.texcoords = texcoord;
                ++vertexIt;
            }
            break;
        default:
            std::cout << "Warning: unsupported normal component type: " << (int)texcoordAccessor.componentType << "\n";

            // Prevent uninitialized data
            fillDefaultTexcoords(vertices, baseVertex, texcoordAccessor.count);
            break;
        }
    }

}

SceneData::SceneData(glTF::Asset const & asset, HDRIData const & environmentMap, WGPUDevice device, WGPUQueue queue,
    WGPUBindGroupLayout geometryBindGroupLayout, WGPUBindGroupLayout materialBindGroupLayout)
{
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    std::vector<Material> materials;

    // Add default rough diffuse material
    materials.push_back({
        .baseColorFactorAndTransmission = glm::vec4(1.f, 1.f, 1.f, 0.f),
        .metallicRoughnessFactorAndIor = glm::vec4(0.f, 1.f, 0.f, 1.5f),
        .emissiveFactor = glm::vec4(0.f),
    });

    for (auto const & node : asset.nodes)
    {
        if (!node.mesh) continue;

        glm::mat3 normalMatrix = glm::inverse(glm::transpose(glm::mat3(node.matrix)));

        for (auto const & primitive : asset.meshes[*node.mesh].primitives)
        {
            if (primitive.mode != glTF::Primitive::Mode::Triangles)
            {
                std::cout << "Warning: only 'triangles' primitive mode is supported\n";
                continue;
            }

            if (!primitive.attributes.position)
            {
                std::cout << "Warning: cannot render a primitive without positions\n";
                continue;
            }

            // Fetch all accessors

            glTF::Accessor const * indexAccessor = nullptr;
            glTF::Accessor const * positionAccessor = nullptr;
            glTF::Accessor const * normalAccessor = nullptr;
            glTF::Accessor const * texcoordAccessor = nullptr;

            if (primitive.indices) indexAccessor = &asset.accessors[*primitive.indices];

            positionAccessor = &asset.accessors[*primitive.attributes.position];

            if (primitive.attributes.normal) normalAccessor = &asset.accessors[*primitive.attributes.normal];
            if (primitive.attributes.texcoord) texcoordAccessor = &asset.accessors[*primitive.attributes.texcoord];

            // Read indices

            std::uint32_t baseIndex = indices.size();
            std::uint32_t baseVertex = vertices.size();
            std::uint32_t indexCount = indexAccessor ? indexAccessor->count : positionAccessor->count;

            if (indexAccessor)
                readIndices(asset, *indexAccessor, indices, baseVertex);
            else
                fillIndices(*positionAccessor, indices, baseVertex);

            // Preallocate vertices

            vertices.resize(vertices.size() + positionAccessor->count);

            // Read positions
            readPositions(asset, *positionAccessor, vertices, baseVertex);

            // Read normals
            if (normalAccessor)
                readNormals(asset, *normalAccessor, vertices, baseVertex);
            else
                reconstructNormals(vertices, indices, baseVertex, baseIndex, positionAccessor->count, indexCount);

            // Read texture coordinates
            if (texcoordAccessor)
                readTexcoords(asset, *texcoordAccessor, vertices, baseVertex);
            else
                fillDefaultTexcoords(vertices, baseVertex, positionAccessor->count);

            std::uint32_t materialID = primitive.material ? 1 + *primitive.material : 0;

            for (std::uint32_t i = 0; i < positionAccessor->count; ++i)
            {
                auto & v = vertices[baseVertex + i];

                v.position = glm::vec3(node.matrix * glm::vec4(v.position, 1.f));
                v.attributes.normal = glm::normalize(normalMatrix * v.attributes.normal);
                v.attributes.materialID = materialID;
            }
        }
    }

    glm::uvec2 maxAlbedoTextureSize(1);

    struct Image
    {
        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t const * pixels;
    };

    std::vector<Image> albedoImages;
    std::unordered_map<std::uint32_t, std::uint32_t> glTFImageToAlbedoArrayLayer;

    albedoImages.push_back({
        .width = 1,
        .height = 1,
        .pixels = nullptr,
    });

    for (auto const & materialIn : asset.materials)
    {
        auto & material = materials.emplace_back();
        material.baseColorFactorAndTransmission = glm::vec4(glm::vec3(materialIn.baseColorFactor), materialIn.transmission);
        material.metallicRoughnessFactorAndIor = glm::vec4(0.f, materialIn.roughnessFactor, materialIn.metallicFactor, materialIn.ior);
        material.emissiveFactor = glm::vec4(materialIn.emissiveFactor, 1.f);
        material.textureLayers = glm::uvec4(0);

        if (materialIn.baseColorTexture)
        {
            if (auto sourceImage = asset.textures[*materialIn.baseColorTexture].source)
            {
                if (glTFImageToAlbedoArrayLayer.contains(*sourceImage))
                {
                    material.textureLayers.x = glTFImageToAlbedoArrayLayer.at(*sourceImage);
                }
                else
                {
                    auto const & image = asset.images[*sourceImage];
                    maxAlbedoTextureSize = glm::max(maxAlbedoTextureSize, glm::uvec2(image.width, image.height));

                    glTFImageToAlbedoArrayLayer[*sourceImage] = albedoImages.size();
                    material.textureLayers.x = albedoImages.size();

                    albedoImages.push_back({
                        .width = image.width,
                        .height = image.height,
                        .pixels = image.data.data(),
                    });
                }
            }
        }
    }

    std::vector<std::uint32_t> whitePixels(maxAlbedoTextureSize.x * maxAlbedoTextureSize.y, 0xffffffffu);
    albedoImages[0].width = maxAlbedoTextureSize.x;
    albedoImages[0].height = maxAlbedoTextureSize.y;
    albedoImages[0].pixels = whitePixels.data();

    std::vector<AABB> triangleAABB(indices.size() / 3);
    for (std::uint32_t i = 0; i < triangleAABB.size(); ++i)
    {
        triangleAABB[i].extend(vertices[indices[3 * i + 0]].position);
        triangleAABB[i].extend(vertices[indices[3 * i + 1]].position);
        triangleAABB[i].extend(vertices[indices[3 * i + 2]].position);
    }

    BVH bvh = buildBVH(triangleAABB);

    {
        // Instead of storing triangleID's per BVH node, store triangles
        // themselves (as index triples), thereby removing the need for
        // extra indirection in the shader

        std::vector<std::uint32_t> sortedIndices;
        for (auto triangleID : bvh.triangleIDs)
        {
            sortedIndices.push_back(indices[3 * triangleID + 0]);
            sortedIndices.push_back(indices[3 * triangleID + 1]);
            sortedIndices.push_back(indices[3 * triangleID + 2]);
        }
        indices = std::move(sortedIndices);
    }

    {
        // Instead of using indexing, store triangles as vertex triples directly,
        // removing another indirection in the shader

        std::vector<Vertex> deindexedVertices;
        for (std::uint32_t i = 0; i < indices.size(); i += 3)
        {
            deindexedVertices.push_back(vertices[indices[i + 0]]);
            deindexedVertices.push_back(vertices[indices[i + 1]]);
            deindexedVertices.push_back(vertices[indices[i + 2]]);
        }

        vertices = std::move(deindexedVertices);
    }

    std::vector<glm::vec4> vertexPositions;
    std::vector<VertexAttributes> vertexAttributes;
    for (auto const & v : vertices)
    {
        vertexPositions.push_back(glm::vec4(v.position, 1.f));
        vertexAttributes.push_back(v.attributes);
    }

    std::vector<std::uint32_t> emissiveTriangles;
    for (std::uint32_t i = 0; i < vertexAttributes.size(); i += 3)
    {
        if (glm::lMaxNorm(glm::vec3(materials[vertexAttributes[i].materialID].emissiveFactor)) > 0.f)
        {
            emissiveTriangles.push_back(i / 3);
        }
    }

    std::vector<AABB> emissiveTriangleAABB(emissiveTriangles.size());
    for (std::uint32_t i = 0; i < emissiveTriangleAABB.size(); ++i)
    {
        auto triangleID = emissiveTriangles[i];
        emissiveTriangleAABB[i].extend(vertices[3 * triangleID + 0].position);
        emissiveTriangleAABB[i].extend(vertices[3 * triangleID + 1].position);
        emissiveTriangleAABB[i].extend(vertices[3 * triangleID + 2].position);
    }

    BVH emissiveBvh = buildBVH(emissiveTriangleAABB);

    {
        // Instead of storing triangleID's per light BVH node, store triangles
        // themselves, thereby removing the need for extra indirection in the shader

        std::vector<std::uint32_t> sortedEmissiveTriangles;

        // First element is actually the array size, see geometry.wgsl
        sortedEmissiveTriangles.push_back(emissiveBvh.triangleIDs.size());

        for (auto triangleIndex : emissiveBvh.triangleIDs)
            sortedEmissiveTriangles.push_back(emissiveTriangles[triangleIndex]);

        // Prevent the triangle buffer from being empty
        if (emissiveBvh.triangleIDs.empty())
            sortedEmissiveTriangles.push_back(0);

        emissiveTriangles = std::move(sortedEmissiveTriangles);
    }

    WGPUBufferDescriptor vertexPositionsBufferDescriptor;
    vertexPositionsBufferDescriptor.nextInChain = nullptr;
    vertexPositionsBufferDescriptor.label = nullptr;
    vertexPositionsBufferDescriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex | WGPUBufferUsage_Storage;
    vertexPositionsBufferDescriptor.size = vertexPositions.size() * sizeof(vertexPositions[0]);
    vertexPositionsBufferDescriptor.mappedAtCreation = false;

    vertexPositionsBuffer_ = wgpuDeviceCreateBuffer(device, &vertexPositionsBufferDescriptor);
    wgpuQueueWriteBuffer(queue, vertexPositionsBuffer_, 0, vertexPositions.data(), vertexPositionsBufferDescriptor.size);

    WGPUBufferDescriptor vertexAttributesBufferDescriptor;
    vertexAttributesBufferDescriptor.nextInChain = nullptr;
    vertexAttributesBufferDescriptor.label = nullptr;
    vertexAttributesBufferDescriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex | WGPUBufferUsage_Storage;
    vertexAttributesBufferDescriptor.size = vertexAttributes.size() * sizeof(vertexAttributes[0]);
    vertexAttributesBufferDescriptor.mappedAtCreation = false;

    vertexAttributesBuffer_ = wgpuDeviceCreateBuffer(device, &vertexAttributesBufferDescriptor);
    wgpuQueueWriteBuffer(queue, vertexAttributesBuffer_, 0, vertexAttributes.data(), vertexAttributesBufferDescriptor.size);

    WGPUBufferDescriptor materialBufferDescriptor;
    materialBufferDescriptor.nextInChain = nullptr;
    materialBufferDescriptor.label = nullptr;
    materialBufferDescriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage;
    materialBufferDescriptor.size = materials.size() * sizeof(materials[0]);
    materialBufferDescriptor.mappedAtCreation = false;

    materialBuffer_ = wgpuDeviceCreateBuffer(device, &materialBufferDescriptor);
    wgpuQueueWriteBuffer(queue, materialBuffer_, 0, materials.data(), materialBufferDescriptor.size);

    WGPUBufferDescriptor bvhNodesBufferDescriptor;
    bvhNodesBufferDescriptor.nextInChain = nullptr;
    bvhNodesBufferDescriptor.label = nullptr;
    bvhNodesBufferDescriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage;
    bvhNodesBufferDescriptor.size = bvh.nodes.size() * sizeof(bvh.nodes[0]);
    bvhNodesBufferDescriptor.mappedAtCreation = false;

    bvhNodesBuffer_ = wgpuDeviceCreateBuffer(device, &bvhNodesBufferDescriptor);
    wgpuQueueWriteBuffer(queue, bvhNodesBuffer_, 0, bvh.nodes.data(), bvhNodesBufferDescriptor.size);

    WGPUBufferDescriptor emissiveTrianglesBufferDescriptor;
    emissiveTrianglesBufferDescriptor.nextInChain = nullptr;
    emissiveTrianglesBufferDescriptor.label = nullptr;
    emissiveTrianglesBufferDescriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage;
    emissiveTrianglesBufferDescriptor.size = emissiveTriangles.size() * sizeof(emissiveTriangles[0]);
    emissiveTrianglesBufferDescriptor.mappedAtCreation = false;

    emissiveTrianglesBuffer_ = wgpuDeviceCreateBuffer(device, &emissiveTrianglesBufferDescriptor);
    wgpuQueueWriteBuffer(queue, emissiveTrianglesBuffer_, 0, emissiveTriangles.data(), emissiveTrianglesBufferDescriptor.size);

    WGPUBufferDescriptor emissiveBvhNodesBufferDescriptor;
    emissiveBvhNodesBufferDescriptor.nextInChain = nullptr;
    emissiveBvhNodesBufferDescriptor.label = nullptr;
    emissiveBvhNodesBufferDescriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage;
    emissiveBvhNodesBufferDescriptor.size = emissiveBvh.nodes.size() * sizeof(emissiveBvh.nodes[0]);
    emissiveBvhNodesBufferDescriptor.mappedAtCreation = false;

    emissiveBvhNodesBuffer_ = wgpuDeviceCreateBuffer(device, &emissiveBvhNodesBufferDescriptor);
    wgpuQueueWriteBuffer(queue, emissiveBvhNodesBuffer_, 0, emissiveBvh.nodes.data(), emissiveBvhNodesBufferDescriptor.size);

    vertexCount_ = vertices.size();

    WGPUSamplerDescriptor samplerDescriptor;
    samplerDescriptor.nextInChain = nullptr;
    samplerDescriptor.label = nullptr;
    samplerDescriptor.addressModeU = WGPUAddressMode_Repeat;
    samplerDescriptor.addressModeV = WGPUAddressMode_Repeat;
    samplerDescriptor.addressModeW = WGPUAddressMode_Repeat;
    samplerDescriptor.magFilter = WGPUFilterMode_Linear;
    samplerDescriptor.minFilter = WGPUFilterMode_Linear;
    samplerDescriptor.mipmapFilter = WGPUMipmapFilterMode_Nearest;
    samplerDescriptor.lodMinClamp = 0.f;
    samplerDescriptor.lodMaxClamp = 0.f;
    samplerDescriptor.compare = WGPUCompareFunction_Undefined;
    samplerDescriptor.maxAnisotropy = 1;

    sampler_ = wgpuDeviceCreateSampler(device, &samplerDescriptor);

    WGPUTextureDescriptor albedoTextureDescriptor;
    albedoTextureDescriptor.nextInChain = nullptr;
    albedoTextureDescriptor.label = nullptr;
    albedoTextureDescriptor.usage = WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding;
    albedoTextureDescriptor.dimension = WGPUTextureDimension_2D;
    albedoTextureDescriptor.size = {maxAlbedoTextureSize.x, maxAlbedoTextureSize.y, (std::uint32_t)albedoImages.size()};
    albedoTextureDescriptor.format = WGPUTextureFormat_RGBA8UnormSrgb;
    albedoTextureDescriptor.mipLevelCount = 1;
    albedoTextureDescriptor.sampleCount = 1;
    albedoTextureDescriptor.viewFormatCount = 0;
    albedoTextureDescriptor.viewFormats = nullptr;
    albedoTexture_ = wgpuDeviceCreateTexture(device, &albedoTextureDescriptor);

    WGPUTextureViewDescriptor albedoTextureViewDescriptor;
    albedoTextureViewDescriptor.nextInChain = nullptr;
    albedoTextureViewDescriptor.label = nullptr;
    albedoTextureViewDescriptor.format = WGPUTextureFormat_RGBA8UnormSrgb;
    albedoTextureViewDescriptor.dimension = WGPUTextureViewDimension_2DArray;
    albedoTextureViewDescriptor.baseMipLevel = 0;
    albedoTextureViewDescriptor.mipLevelCount = 1;
    albedoTextureViewDescriptor.baseArrayLayer = 0;
    albedoTextureViewDescriptor.arrayLayerCount = albedoImages.size();
    albedoTextureViewDescriptor.aspect = WGPUTextureAspect_All;
    albedoTextureView_ = wgpuTextureCreateView(albedoTexture_, &albedoTextureViewDescriptor);

    for (std::uint32_t layer = 0; layer < albedoImages.size(); ++layer)
    {
        WGPUImageCopyTexture textureDestination;
        textureDestination.nextInChain = nullptr;
        textureDestination.texture = albedoTexture_;
        textureDestination.mipLevel = 0;
        textureDestination.origin = {0, 0, layer};
        textureDestination.aspect = WGPUTextureAspect_All;

        WGPUTextureDataLayout textureDataLayout;
        textureDataLayout.nextInChain = nullptr;
        textureDataLayout.offset = 0;
        textureDataLayout.bytesPerRow = albedoImages[layer].width * 4;
        textureDataLayout.rowsPerImage = albedoImages[layer].height;

        WGPUExtent3D textureWriteSize;
        textureWriteSize.width = albedoImages[layer].width;
        textureWriteSize.height = albedoImages[layer].height;
        textureWriteSize.depthOrArrayLayers = 1;

        wgpuQueueWriteTexture(queue, &textureDestination, albedoImages[layer].pixels, albedoImages[layer].width * albedoImages[layer].height * 4, &textureDataLayout, &textureWriteSize);
    }

    WGPUTextureDescriptor environmentTextureDescriptor;
    environmentTextureDescriptor.nextInChain = nullptr;
    environmentTextureDescriptor.label = nullptr;
    environmentTextureDescriptor.usage = WGPUTextureUsage_CopyDst | WGPUTextureUsage_StorageBinding;
    environmentTextureDescriptor.dimension = WGPUTextureDimension_2D;
    environmentTextureDescriptor.size = {environmentMap.width, environmentMap.height, 1};
    environmentTextureDescriptor.format = WGPUTextureFormat_RGBA32Float;
    environmentTextureDescriptor.mipLevelCount = 1;
    environmentTextureDescriptor.sampleCount = 1;
    environmentTextureDescriptor.viewFormatCount = 0;
    environmentTextureDescriptor.viewFormats = nullptr;
    environmentTexture_ = wgpuDeviceCreateTexture(device, &environmentTextureDescriptor);

    WGPUImageCopyTexture environmentTextureDestination;
    environmentTextureDestination.nextInChain = nullptr;
    environmentTextureDestination.texture = environmentTexture_;
    environmentTextureDestination.mipLevel = 0;
    environmentTextureDestination.origin = {0, 0, 0};
    environmentTextureDestination.aspect = WGPUTextureAspect_All;

    WGPUTextureDataLayout environmentTextureDataLayout;
    environmentTextureDataLayout.nextInChain = nullptr;
    environmentTextureDataLayout.offset = 0;
    environmentTextureDataLayout.bytesPerRow = environmentMap.width * sizeof(float) * 4;
    environmentTextureDataLayout.rowsPerImage = environmentMap.height;

    WGPUExtent3D environmentTextureWriteSize;
    environmentTextureWriteSize.width = environmentMap.width;
    environmentTextureWriteSize.height = environmentMap.height;
    environmentTextureWriteSize.depthOrArrayLayers = 1;

    wgpuQueueWriteTexture(queue, &environmentTextureDestination, environmentMap.pixels.data(), environmentMap.pixels.size() * sizeof(float), &environmentTextureDataLayout, &environmentTextureWriteSize);

    WGPUTextureViewDescriptor environmentTextureViewDescriptor;
    environmentTextureViewDescriptor.nextInChain = nullptr;
    environmentTextureViewDescriptor.label = nullptr;
    environmentTextureViewDescriptor.format = WGPUTextureFormat_RGBA32Float;
    environmentTextureViewDescriptor.dimension = WGPUTextureViewDimension_2D;
    environmentTextureViewDescriptor.baseMipLevel = 0;
    environmentTextureViewDescriptor.mipLevelCount = 1;
    environmentTextureViewDescriptor.baseArrayLayer = 0;
    environmentTextureViewDescriptor.arrayLayerCount = 1;
    environmentTextureViewDescriptor.aspect = WGPUTextureAspect_All;
    environmentTextureView_ = wgpuTextureCreateView(environmentTexture_, &environmentTextureViewDescriptor);

    geometryBindGroup_ = createGeometryBindGroup(device, geometryBindGroupLayout, vertexPositionsBuffer_, vertexAttributesBuffer_,
        bvhNodesBuffer_, emissiveTrianglesBuffer_, emissiveBvhNodesBuffer_);
    materialBindGroup_ = createMaterialBindGroup(device, materialBindGroupLayout, materialBuffer_, sampler_, albedoTextureView_, environmentTextureView_);
}

SceneData::~SceneData()
{
    wgpuBindGroupRelease(materialBindGroup_);
    wgpuBindGroupRelease(geometryBindGroup_);

    wgpuTextureViewRelease(environmentTextureView_);
    wgpuTextureRelease(environmentTexture_);

    wgpuBufferRelease(emissiveBvhNodesBuffer_);
    wgpuBufferRelease(emissiveTrianglesBuffer_);
    wgpuBufferRelease(bvhNodesBuffer_);
    wgpuBufferRelease(materialBuffer_);
    wgpuBufferRelease(vertexAttributesBuffer_);
    wgpuBufferRelease(vertexPositionsBuffer_);
}
