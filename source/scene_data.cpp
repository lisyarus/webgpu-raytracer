#include <webgpu-raytracer/scene_data.hpp>
#include <webgpu-raytracer/gltf_iterator.hpp>
#include <webgpu-raytracer/material_bind_group.hpp>
#include <webgpu-raytracer/geometry_bind_group.hpp>

#include <glm/glm.hpp>

#include <iostream>

namespace
{

    struct Vertex
    {
        glm::vec3 position;
        std::uint32_t padding = 0;
        glm::vec3 normal;
        std::uint32_t materialID;
    };

    struct Material
    {
        glm::vec4 baseColorFactor;
        glm::vec4 metallicRoughnessFactor;
        glm::vec4 emissiveFactor;
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
                vertexIt->normal = normal;
                ++vertexIt;
            }
            break;
        default:
            std::cout << "Warning: unsupported normal component type: " << (int)normalAccessor.componentType << "\n";

            // Prevent uninitialized data
            for (std::uint32_t i = 0; i < normalAccessor.count; ++i)
            {
                vertexIt->normal = glm::vec3(0.f, 0.f, 1.f);
                ++vertexIt;
            }
            break;
        }
    }

    void reconstructNormals(std::vector<Vertex> & vertices, std::vector<std::uint32_t> const & indices, std::uint32_t baseVertex, std::uint32_t baseIndex,
        std::uint32_t vertexCount, std::uint32_t indexCount)
    {
        for (std::uint32_t i = 0; i < vertexCount; ++i)
            vertices[baseVertex + i].normal = glm::vec3(0.f);

        // Compute average adjacent triangle normal
        for (std::uint32_t i = 0; i < indexCount; i += 3)
        {
            auto & v0 = vertices[baseVertex + indices[baseIndex + i + 0]];
            auto & v1 = vertices[baseVertex + indices[baseIndex + i + 1]];
            auto & v2 = vertices[baseVertex + indices[baseIndex + i + 2]];

            auto normal = glm::normalize(glm::cross(v1.position - v0.position, v2.position - v0.position));

            v0.normal += normal;
            v1.normal += normal;
            v2.normal += normal;
        }

        for (std::uint32_t i = 0; i < vertexCount; ++i)
        {
            auto & v = vertices[baseVertex + i];
            v.normal = glm::normalize(v.normal);
        }
    }

}

SceneData::SceneData(glTF::Asset const & asset, WGPUDevice device, WGPUQueue queue,
    WGPUBindGroupLayout geometryBindGroupLayout, WGPUBindGroupLayout materialBindGroupLayout)
{
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    std::vector<Material> materials;

    // Add default rough diffuse material
    materials.push_back({
        .baseColorFactor = glm::vec4(1.f),
        .metallicRoughnessFactor = glm::vec4(1.f, 1.f, 0.f, 1.f),
        .emissiveFactor = glm::vec4(1.f),
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

            if (primitive.indices) indexAccessor = &asset.accessors[*primitive.indices];

            positionAccessor = &asset.accessors[*primitive.attributes.position];

            if (primitive.attributes.normal) normalAccessor = &asset.accessors[*primitive.attributes.normal];

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

            std::uint32_t materialID = primitive.material ? 1 + *primitive.material : 0;

            for (std::uint32_t i = 0; i < positionAccessor->count; ++i)
            {
                auto & v = vertices[baseVertex + i];

                v.position = glm::vec3(node.matrix * glm::vec4(v.position, 1.f));
                v.normal = glm::normalize(normalMatrix * v.normal);
                v.materialID = materialID;
            }
        }
    }

    for (auto const & materialIn : asset.materials)
    {
        auto & material = materials.emplace_back();
        material.baseColorFactor = materialIn.baseColorFactor;
        material.metallicRoughnessFactor = glm::vec4(1.f, materialIn.roughnessFactor, materialIn.metallicFactor, 1.f);
        material.emissiveFactor = glm::vec4(materialIn.emissiveFactor, 1.f);
    }

    WGPUBufferDescriptor vertexBufferDescriptor;
    vertexBufferDescriptor.nextInChain = nullptr;
    vertexBufferDescriptor.label = nullptr;
    vertexBufferDescriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex | WGPUBufferUsage_Storage;
    vertexBufferDescriptor.size = vertices.size() * sizeof(vertices[0]);
    vertexBufferDescriptor.mappedAtCreation = false;

    vertexBuffer_ = wgpuDeviceCreateBuffer(device, &vertexBufferDescriptor);
    wgpuQueueWriteBuffer(queue, vertexBuffer_, 0, vertices.data(), vertexBufferDescriptor.size);

    WGPUBufferDescriptor indexBufferDescriptor;
    indexBufferDescriptor.nextInChain = nullptr;
    indexBufferDescriptor.label = nullptr;
    indexBufferDescriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index | WGPUBufferUsage_Storage;
    indexBufferDescriptor.size = indices.size() * sizeof(indices[0]);
    indexBufferDescriptor.mappedAtCreation = false;

    indexBuffer_ = wgpuDeviceCreateBuffer(device, &indexBufferDescriptor);
    wgpuQueueWriteBuffer(queue, indexBuffer_, 0, indices.data(), indexBufferDescriptor.size);

    WGPUBufferDescriptor materialBufferDescriptor;
    materialBufferDescriptor.nextInChain = nullptr;
    materialBufferDescriptor.label = nullptr;
    materialBufferDescriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage;
    materialBufferDescriptor.size = materials.size() * sizeof(materials[0]);
    materialBufferDescriptor.mappedAtCreation = false;

    materialBuffer_ = wgpuDeviceCreateBuffer(device, &materialBufferDescriptor);
    wgpuQueueWriteBuffer(queue, materialBuffer_, 0, materials.data(), materialBufferDescriptor.size);

    indexCount_ = indices.size();

    geometryBindGroup_ = createGeometryBindGroup(device, geometryBindGroupLayout, vertexBuffer_, indexBuffer_);
    materialBindGroup_ = createMaterialBindGroup(device, materialBindGroupLayout, materialBuffer_);
}

SceneData::~SceneData()
{
    wgpuBindGroupRelease(materialBindGroup_);
    wgpuBindGroupRelease(geometryBindGroup_);

    wgpuBufferRelease(materialBuffer_);
    wgpuBufferRelease(indexBuffer_);
    wgpuBufferRelease(vertexBuffer_);
}
