#include <webgpu-raytracer/scene_data.hpp>
#include <webgpu-raytracer/gltf_iterator.hpp>
#include <webgpu-raytracer/material_bind_group.hpp>
#include <webgpu-raytracer/geometry_bind_group.hpp>
#include <webgpu-raytracer/bvh.hpp>
#include <webgpu-raytracer/alias.hpp>
#include <webgpu-raytracer/timer.hpp>
#include <stb_image.h>
#include <mikktspace.h>

#include <glm/glm.hpp>

#include <iostream>

namespace
{

    struct VertexAttributes
    {
        glm::vec3 normal;
        std::uint32_t materialID;
        glm::vec4 tangent;
        glm::vec2 texcoords;
        char padding[8];
    };

    static_assert(sizeof(VertexAttributes) == 48);

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
        // uvec4(albedo, material, 0, 0)
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
        auto vertexBegin = vertices.begin() + baseVertex;
        auto vertexEnd = vertexBegin + count;

        for (auto vertexIt = vertexBegin; vertexIt != vertexEnd; ++vertexIt)
            vertexIt->attributes.texcoords = {0.5f, 0.5f};
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

    void readTangents(glTF::Asset const & asset, glTF::Accessor const & tangentAccessor, std::vector<Vertex> & vertices, std::uint32_t baseVertex)
    {
        auto vertexIt = vertices.begin() + baseVertex;

        switch (tangentAccessor.componentType)
        {
        case glTF::Accessor::ComponentType::Float:
            for (auto tangent : glTF::AccessorRange<glm::vec4>(asset, tangentAccessor))
            {
                vertexIt->attributes.tangent = tangent;
                ++vertexIt;
            }
            break;
        default:
            std::cout << "Warning: unsupported tangent component type: " << (int)tangentAccessor.componentType << "\n";

            // Prevent uninitialized data
            for (std::uint32_t i = 0; i < tangentAccessor.count; ++i)
            {
                vertexIt->attributes.tangent = glm::vec4(1.f, 0.f, 0.f, 1.f);
                ++vertexIt;
            }
            break;
        }
    }

    void fillDefaultTangents(std::vector<Vertex> & vertices, std::uint32_t baseVertex, std::uint32_t count)
    {
        auto vertexBegin = vertices.begin() + baseVertex;
        auto vertexEnd = vertexBegin + count;

        for (auto vertexIt = vertexBegin; vertexIt != vertexEnd; ++vertexIt)
        {
            glm::vec3 tangent;
            if (std::abs(vertexIt->attributes.normal.z) < 0.5f)
                tangent = glm::cross(vertexIt->attributes.normal, glm::vec3(0.f, 0.f, 1.f));
            else
                tangent = glm::cross(vertexIt->attributes.normal, glm::vec3(1.f, 0.f, 0.f));
            vertexIt->attributes.tangent = glm::vec4(glm::normalize(tangent), 1.f);
        }
    }

    void reconstructTangents(std::vector<Vertex> & vertices, std::vector<std::uint32_t> const & indices, std::uint32_t baseVertex, std::uint32_t baseIndex,
        std::uint32_t vertexCount, std::uint32_t indexCount)
    {
        struct Context
        {
            Vertex * vertices;
            std::uint32_t const * indices;
            std::uint32_t vertexCount;
            std::uint32_t indexCount;
        };

        Context context
        {
            .vertices = vertices.data(),
            .indices = indices.data() + baseIndex,
            .vertexCount = vertexCount,
            .indexCount = indexCount,
        };

        SMikkTSpaceInterface mikkTSpaceInterface
        {
            .m_getNumFaces = [](SMikkTSpaceContext const * pContext) -> int
            {
                return ((Context *)(pContext->m_pUserData))->indexCount / 3;
            },
            .m_getNumVerticesOfFace = [](SMikkTSpaceContext const *, int) -> int
            {
                return 3;
            },
            .m_getPosition = [](SMikkTSpaceContext const * pContext, float * fvPosOut, int iFace, int iVert)
            {
                auto context = (Context *)(pContext->m_pUserData);
                auto const & vertex = context->vertices[context->indices[3 * iFace + iVert]];
                fvPosOut[0] = vertex.position.x;
                fvPosOut[1] = vertex.position.y;
                fvPosOut[2] = vertex.position.z;
            },
            .m_getNormal = [](SMikkTSpaceContext const * pContext, float * fvNormOut, int iFace, int iVert)
            {
                auto context = (Context *)(pContext->m_pUserData);
                auto const & vertex = context->vertices[context->indices[3 * iFace + iVert]];
                fvNormOut[0] = vertex.attributes.normal.x;
                fvNormOut[1] = vertex.attributes.normal.y;
                fvNormOut[2] = vertex.attributes.normal.z;
            },
            .m_getTexCoord = [](SMikkTSpaceContext const * pContext, float * fvTexcOut, int iFace, int iVert)
            {
                auto context = (Context *)(pContext->m_pUserData);
                auto const & vertex = context->vertices[context->indices[3 * iFace + iVert]];
                fvTexcOut[0] = vertex.attributes.texcoords.x;
                fvTexcOut[1] = vertex.attributes.texcoords.y;
            },
            .m_setTSpaceBasic = [](SMikkTSpaceContext const * pContext, float const * fvTangent, float fSign, int iFace, int iVert)
            {
                auto context = (Context *)(pContext->m_pUserData);
                auto & vertex = context->vertices[context->indices[3 * iFace + iVert]];
                vertex.attributes.tangent.x = fvTangent[0];
                vertex.attributes.tangent.y = fvTangent[1];
                vertex.attributes.tangent.z = fvTangent[2];
                vertex.attributes.tangent.w = fSign;
            },
            .m_setTSpace = nullptr,
        };

        SMikkTSpaceContext mikkTSpaceContext
        {
            .m_pInterface = &mikkTSpaceInterface,
            .m_pUserData = &context,
        };

        genTangSpaceDefault(&mikkTSpaceContext);
    }

    struct Image
    {
        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t const * pixels;
    };

    std::vector<std::uint32_t> rescaleImage(Image & image, glm::uvec2 const & targetSize)
    {
        if (image.width == targetSize.x && image.height == targetSize.y)
            return {};

        std::vector<std::uint32_t> scaledPixels(targetSize.x * targetSize.y);

        for (std::uint32_t y = 0; y < targetSize.y; ++y)
        {
            for (std::uint32_t x = 0; x < targetSize.x; ++x)
            {
                auto sx = (x * image.width) / targetSize.x;
                auto sy = (y * image.height) / targetSize.y;
                scaledPixels[x + y * targetSize.x] = image.pixels[sx + sy * image.width];
            }
        }

        image.width = targetSize.x;
        image.height = targetSize.y;
        image.pixels = scaledPixels.data();

        return scaledPixels;
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

        glm::mat3 normalMatrix = glm::inverse(glm::transpose(glm::mat3(node.globalMatrix)));

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
            glTF::Accessor const * tangentAccessor = nullptr;

            if (primitive.indices) indexAccessor = &asset.accessors[*primitive.indices];

            positionAccessor = &asset.accessors[*primitive.attributes.position];

            if (primitive.attributes.normal) normalAccessor = &asset.accessors[*primitive.attributes.normal];
            if (primitive.attributes.texcoord) texcoordAccessor = &asset.accessors[*primitive.attributes.texcoord];
            if (primitive.attributes.tangent) tangentAccessor = &asset.accessors[*primitive.attributes.tangent];

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

            // Read tangents
            if (tangentAccessor)
                readTangents(asset, *tangentAccessor, vertices, baseVertex);
            else if (texcoordAccessor)
                reconstructTangents(vertices, indices, baseVertex, baseIndex, positionAccessor->count, indexCount);
            else
                fillDefaultTangents(vertices, baseVertex, positionAccessor->count);

            std::uint32_t materialID = primitive.material ? 1 + *primitive.material : 0;

            for (std::uint32_t i = 0; i < positionAccessor->count; ++i)
            {
                auto & v = vertices[baseVertex + i];

                v.position = glm::vec3(node.globalMatrix * glm::vec4(v.position, 1.f));
                v.attributes.normal = glm::normalize(normalMatrix * v.attributes.normal);
                v.attributes.tangent = glm::vec4(glm::normalize(glm::mat3(node.globalMatrix) * glm::vec3(v.attributes.tangent)), v.attributes.tangent.w);
                v.attributes.materialID = materialID;
            }
        }
    }

    glm::uvec2 maxAlbedoTextureSize(1);
    glm::uvec2 maxMaterialTextureSize(1);
    glm::uvec2 maxNormalTextureSize(1);

    std::vector<Image> albedoImages;
    std::vector<Image> materialImages;
    std::vector<Image> normalImages;

    std::unordered_map<std::uint32_t, std::uint32_t> glTFImageToAlbedoArrayLayer;
    std::unordered_map<std::uint32_t, std::uint32_t> glTFImageToMaterialArrayLayer;
    std::unordered_map<std::uint32_t, std::uint32_t> glTFImageToNormalArrayLayer;

    albedoImages.push_back({
        .width = 1,
        .height = 1,
        .pixels = nullptr,
    });

    materialImages.push_back({
        .width = 1,
        .height = 1,
        .pixels = nullptr,
    });

    normalImages.push_back({
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

        if (materialIn.metallicRoughnessTexture)
        {
            if (auto sourceImage = asset.textures[*materialIn.metallicRoughnessTexture].source)
            {
                if (glTFImageToMaterialArrayLayer.contains(*sourceImage))
                {
                    material.textureLayers.y = glTFImageToMaterialArrayLayer.at(*sourceImage);
                }
                else
                {
                    auto const & image = asset.images[*sourceImage];
                    maxMaterialTextureSize = glm::max(maxMaterialTextureSize, glm::uvec2(image.width, image.height));

                    glTFImageToMaterialArrayLayer[*sourceImage] = materialImages.size();
                    material.textureLayers.y = materialImages.size();

                    materialImages.push_back({
                        .width = image.width,
                        .height = image.height,
                        .pixels = image.data.data(),
                    });
                }
            }
        }

        if (materialIn.normalTexture)
        {
            if (auto sourceImage = asset.textures[*materialIn.normalTexture].source)
            {
                if (glTFImageToNormalArrayLayer.contains(*sourceImage))
                {
                    material.textureLayers.z = glTFImageToNormalArrayLayer.at(*sourceImage);
                }
                else
                {
                    auto const & image = asset.images[*sourceImage];
                    maxNormalTextureSize = glm::max(maxNormalTextureSize, glm::uvec2(image.width, image.height));

                    glTFImageToNormalArrayLayer[*sourceImage] = normalImages.size();
                    material.textureLayers.z = normalImages.size();

                    normalImages.push_back({
                        .width = image.width,
                        .height = image.height,
                        .pixels = image.data.data(),
                    });
                }
            }
        }
    }

    std::vector<std::uint32_t> whiteAlbedoPixels(maxAlbedoTextureSize.x * maxAlbedoTextureSize.y, 0xffffffffu);
    albedoImages[0].width = maxAlbedoTextureSize.x;
    albedoImages[0].height = maxAlbedoTextureSize.y;
    albedoImages[0].pixels = whiteAlbedoPixels.data();

    std::vector<std::uint32_t> whiteMaterialPixels(maxMaterialTextureSize.x * maxMaterialTextureSize.y, 0xffffffffu);
    materialImages[0].width = maxMaterialTextureSize.x;
    materialImages[0].height = maxMaterialTextureSize.y;
    materialImages[0].pixels = whiteMaterialPixels.data();

    std::vector<std::uint32_t> blueNormalPixels(maxNormalTextureSize.x * maxNormalTextureSize.y, 0xffff7f7f);
    normalImages[0].width = maxNormalTextureSize.x;
    normalImages[0].height = maxNormalTextureSize.y;
    normalImages[0].pixels = blueNormalPixels.data();

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
    std::vector<float> emissiveTriangleWeight(emissiveTriangles.size());
    float emissiveTrianglesTotalWeight = 0.f;

    for (std::uint32_t i = 0; i < emissiveTriangleAABB.size(); ++i)
    {
        auto triangleID = emissiveTriangles[i];

        auto v0 = vertices[3 * triangleID + 0].position;
        auto v1 = vertices[3 * triangleID + 1].position;
        auto v2 = vertices[3 * triangleID + 2].position;

        emissiveTriangleAABB[i].extend(v0);
        emissiveTriangleAABB[i].extend(v1);
        emissiveTriangleAABB[i].extend(v2);

        float areaWeight = glm::length(glm::cross(v1 - v0, v2 - v0));

        auto materialID = vertexAttributes[3 * triangleID + 0].materialID;

        // Weight based on percieved luminance
        float emissiveWeight = glm::dot(glm::vec3(0.299f, 0.587f, 0.114f), glm::vec3(materials[materialID].emissiveFactor));

        float weight = areaWeight * emissiveWeight;

        emissiveTriangleWeight[i] = weight;
        emissiveTrianglesTotalWeight += weight;
    }

    for (auto & weight : emissiveTriangleWeight)
        weight /= emissiveTrianglesTotalWeight;

    BVH emissiveBvh = buildBVH(emissiveTriangleAABB);
    auto emissiveAliasTable = generateAlias(emissiveTriangleWeight);

    struct TriangleIndexAndProbability
    {
        std::uint32_t index;
        float probability;
    };

    // Instead of storing triangleID's per light BVH node, store triangles
    // themselves, thereby removing the need for extra indirection in the shader

    std::vector<TriangleIndexAndProbability> sortedEmissiveTriangles;
    std::vector<AliasRecord> sortedEmissiveAliasTable;

    // First element is actually the array size, see geometry.wgsl
    sortedEmissiveTriangles.push_back({(std::uint32_t)emissiveTriangles.size(), 0.f});

    {
        std::vector<std::uint32_t> sortedTrianglesNewID(emissiveTriangles.size());

        for (auto triangleIndex : emissiveBvh.triangleIDs)
        {
            sortedTrianglesNewID[triangleIndex] = sortedEmissiveTriangles.size() - 1;
            sortedEmissiveTriangles.push_back({emissiveTriangles[triangleIndex], emissiveTriangleWeight[triangleIndex]});
        }

        for (auto triangleIndex : emissiveBvh.triangleIDs){
            auto aliasRecord = emissiveAliasTable[triangleIndex];
            sortedEmissiveAliasTable.push_back({
                .probability = aliasRecord.probability,
                .alias = sortedTrianglesNewID[aliasRecord.alias],
            });
        }

        // Prevent the triangle buffer from being empty
        if (emissiveBvh.triangleIDs.empty())
        {
            sortedEmissiveTriangles.push_back({0, 0.f});
            sortedEmissiveAliasTable.push_back({1.f, 0});
        }
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
    emissiveTrianglesBufferDescriptor.size = sortedEmissiveTriangles.size() * sizeof(sortedEmissiveTriangles[0]);
    emissiveTrianglesBufferDescriptor.mappedAtCreation = false;

    emissiveTrianglesBuffer_ = wgpuDeviceCreateBuffer(device, &emissiveTrianglesBufferDescriptor);
    wgpuQueueWriteBuffer(queue, emissiveTrianglesBuffer_, 0, sortedEmissiveTriangles.data(), emissiveTrianglesBufferDescriptor.size);

    WGPUBufferDescriptor emissiveTrianglesAliasBufferDescriptor;
    emissiveTrianglesAliasBufferDescriptor.nextInChain = nullptr;
    emissiveTrianglesAliasBufferDescriptor.label = nullptr;
    emissiveTrianglesAliasBufferDescriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage;
    emissiveTrianglesAliasBufferDescriptor.size = sortedEmissiveAliasTable.size() * sizeof(sortedEmissiveAliasTable[0]);
    emissiveTrianglesAliasBufferDescriptor.mappedAtCreation = false;

    emissiveTrianglesAliasBuffer_ = wgpuDeviceCreateBuffer(device, &emissiveTrianglesAliasBufferDescriptor);
    wgpuQueueWriteBuffer(queue, emissiveTrianglesAliasBuffer_, 0, sortedEmissiveAliasTable.data(), emissiveTrianglesAliasBufferDescriptor.size);

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
        auto & image = albedoImages[layer];

        std::vector<std::uint32_t> scaledPixels = rescaleImage(image, maxAlbedoTextureSize);

        WGPUImageCopyTexture textureDestination;
        textureDestination.nextInChain = nullptr;
        textureDestination.texture = albedoTexture_;
        textureDestination.mipLevel = 0;
        textureDestination.origin = {0, 0, layer};
        textureDestination.aspect = WGPUTextureAspect_All;

        WGPUTextureDataLayout textureDataLayout;
        textureDataLayout.nextInChain = nullptr;
        textureDataLayout.offset = 0;
        textureDataLayout.bytesPerRow = image.width * 4;
        textureDataLayout.rowsPerImage = image.height;

        WGPUExtent3D textureWriteSize;
        textureWriteSize.width = image.width;
        textureWriteSize.height = image.height;
        textureWriteSize.depthOrArrayLayers = 1;

        wgpuQueueWriteTexture(queue, &textureDestination, image.pixels, image.width * image.height * 4, &textureDataLayout, &textureWriteSize);
    }

    WGPUTextureDescriptor materialTextureDescriptor;
    materialTextureDescriptor.nextInChain = nullptr;
    materialTextureDescriptor.label = nullptr;
    materialTextureDescriptor.usage = WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding;
    materialTextureDescriptor.dimension = WGPUTextureDimension_2D;
    materialTextureDescriptor.size = {maxMaterialTextureSize.x, maxMaterialTextureSize.y, (std::uint32_t)materialImages.size()};
    materialTextureDescriptor.format = WGPUTextureFormat_RGBA8Unorm;
    materialTextureDescriptor.mipLevelCount = 1;
    materialTextureDescriptor.sampleCount = 1;
    materialTextureDescriptor.viewFormatCount = 0;
    materialTextureDescriptor.viewFormats = nullptr;
    materialTexture_ = wgpuDeviceCreateTexture(device, &materialTextureDescriptor);

    WGPUTextureViewDescriptor materialTextureViewDescriptor;
    materialTextureViewDescriptor.nextInChain = nullptr;
    materialTextureViewDescriptor.label = nullptr;
    materialTextureViewDescriptor.format = WGPUTextureFormat_RGBA8Unorm;
    materialTextureViewDescriptor.dimension = WGPUTextureViewDimension_2DArray;
    materialTextureViewDescriptor.baseMipLevel = 0;
    materialTextureViewDescriptor.mipLevelCount = 1;
    materialTextureViewDescriptor.baseArrayLayer = 0;
    materialTextureViewDescriptor.arrayLayerCount = materialImages.size();
    materialTextureViewDescriptor.aspect = WGPUTextureAspect_All;
    materialTextureView_ = wgpuTextureCreateView(materialTexture_, &materialTextureViewDescriptor);

    for (std::uint32_t layer = 0; layer < materialImages.size(); ++layer)
    {
        auto & image = materialImages[layer];

        std::vector<std::uint32_t> scaledPixels = rescaleImage(image, maxMaterialTextureSize);

        WGPUImageCopyTexture textureDestination;
        textureDestination.nextInChain = nullptr;
        textureDestination.texture = materialTexture_;
        textureDestination.mipLevel = 0;
        textureDestination.origin = {0, 0, layer};
        textureDestination.aspect = WGPUTextureAspect_All;

        WGPUTextureDataLayout textureDataLayout;
        textureDataLayout.nextInChain = nullptr;
        textureDataLayout.offset = 0;
        textureDataLayout.bytesPerRow = image.width * 4;
        textureDataLayout.rowsPerImage = image.height;

        WGPUExtent3D textureWriteSize;
        textureWriteSize.width = image.width;
        textureWriteSize.height = image.height;
        textureWriteSize.depthOrArrayLayers = 1;

        wgpuQueueWriteTexture(queue, &textureDestination, image.pixels, image.width * image.height * 4, &textureDataLayout, &textureWriteSize);
    }

    WGPUTextureDescriptor normalTextureDescriptor;
    normalTextureDescriptor.nextInChain = nullptr;
    normalTextureDescriptor.label = nullptr;
    normalTextureDescriptor.usage = WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding;
    normalTextureDescriptor.dimension = WGPUTextureDimension_2D;
    normalTextureDescriptor.size = {maxNormalTextureSize.x, maxNormalTextureSize.y, (std::uint32_t)normalImages.size()};
    normalTextureDescriptor.format = WGPUTextureFormat_RGBA8Unorm;
    normalTextureDescriptor.mipLevelCount = 1;
    normalTextureDescriptor.sampleCount = 1;
    normalTextureDescriptor.viewFormatCount = 0;
    normalTextureDescriptor.viewFormats = nullptr;
    normalTexture_ = wgpuDeviceCreateTexture(device, &normalTextureDescriptor);

    WGPUTextureViewDescriptor normalTextureViewDescriptor;
    normalTextureViewDescriptor.nextInChain = nullptr;
    normalTextureViewDescriptor.label = nullptr;
    normalTextureViewDescriptor.format = WGPUTextureFormat_RGBA8Unorm;
    normalTextureViewDescriptor.dimension = WGPUTextureViewDimension_2DArray;
    normalTextureViewDescriptor.baseMipLevel = 0;
    normalTextureViewDescriptor.mipLevelCount = 1;
    normalTextureViewDescriptor.baseArrayLayer = 0;
    normalTextureViewDescriptor.arrayLayerCount = normalImages.size();
    normalTextureViewDescriptor.aspect = WGPUTextureAspect_All;
    normalTextureView_ = wgpuTextureCreateView(normalTexture_, &normalTextureViewDescriptor);

    for (std::uint32_t layer = 0; layer < normalImages.size(); ++layer)
    {
        auto & image = normalImages[layer];

        std::vector<std::uint32_t> scaledPixels = rescaleImage(image, maxNormalTextureSize);

        WGPUImageCopyTexture textureDestination;
        textureDestination.nextInChain = nullptr;
        textureDestination.texture = normalTexture_;
        textureDestination.mipLevel = 0;
        textureDestination.origin = {0, 0, layer};
        textureDestination.aspect = WGPUTextureAspect_All;

        WGPUTextureDataLayout textureDataLayout;
        textureDataLayout.nextInChain = nullptr;
        textureDataLayout.offset = 0;
        textureDataLayout.bytesPerRow = image.width * 4;
        textureDataLayout.rowsPerImage = image.height;

        WGPUExtent3D textureWriteSize;
        textureWriteSize.width = image.width;
        textureWriteSize.height = image.height;
        textureWriteSize.depthOrArrayLayers = 1;

        wgpuQueueWriteTexture(queue, &textureDestination, image.pixels, image.width * image.height * 4, &textureDataLayout, &textureWriteSize);
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
        bvhNodesBuffer_, emissiveTrianglesBuffer_, emissiveTrianglesAliasBuffer_, emissiveBvhNodesBuffer_);
    materialBindGroup_ = createMaterialBindGroup(device, materialBindGroupLayout, materialBuffer_, sampler_,
        albedoTextureView_, materialTextureView_, normalTextureView_, environmentTextureView_);
}

SceneData::~SceneData()
{
    wgpuBindGroupRelease(materialBindGroup_);
    wgpuBindGroupRelease(geometryBindGroup_);

    wgpuTextureViewRelease(environmentTextureView_);
    wgpuTextureRelease(environmentTexture_);

    wgpuTextureViewRelease(normalTextureView_);
    wgpuTextureRelease(normalTexture_);

    wgpuTextureViewRelease(materialTextureView_);
    wgpuTextureRelease(materialTexture_);

    wgpuTextureViewRelease(albedoTextureView_);
    wgpuTextureRelease(albedoTexture_);

    wgpuBufferRelease(emissiveBvhNodesBuffer_);
    wgpuBufferRelease(emissiveTrianglesAliasBuffer_);
    wgpuBufferRelease(emissiveTrianglesBuffer_);
    wgpuBufferRelease(bvhNodesBuffer_);
    wgpuBufferRelease(materialBuffer_);
    wgpuBufferRelease(vertexAttributesBuffer_);
    wgpuBufferRelease(vertexPositionsBuffer_);
}
