#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <string>
#include <vector>
#include <optional>

namespace glTF
{

    struct Buffer
    {
        std::string uri;
        std::uint32_t byteLength;
    };

    struct BufferView
    {
        std::uint32_t buffer;
        std::uint32_t byteOffset;
        std::uint32_t byteLength;
        std::optional<std::uint32_t> byteStride;
    };

    struct Accessor
    {
        enum class ComponentType : std::uint32_t
        {
            Byte = 5120,
            UnsignedByte = 5121,
            Short = 5122,
            UnsignedShort = 5123,
            UnsignedInt = 5125,
            Float = 5126,
        };

        enum class Type : std::uint32_t
        {
            Scalar,
            Vec2,
            Vec3,
            Vec4,
            Mat2,
            Mat3,
            Mat4,
            Unknown,
        };

        std::uint32_t bufferView;
        std::uint32_t byteOffset;
        ComponentType componentType;
        bool normalized;
        std::uint32_t count;
        Type type;
    };

    struct Camera
    {
        float yFov;
        float zNear;
    };

    struct Image
    {
        std::string uri;
    };

    struct Texture
    {
        std::optional<std::uint32_t> source;
    };

    struct Material
    {
        glm::vec4 baseColorFactor;
        std::optional<std::uint32_t> baseColorTexture;

        float metallicFactor;
        float roughnessFactor;
        std::optional<std::uint32_t> metallicRoughnessTexture;

        std::optional<std::uint32_t> normalTexture;

        glm::vec3 emissiveFactor;
        std::optional<std::uint32_t> emissiveTexture;

        bool cloth;
    };

    struct Primitive
    {
        struct Attributes
        {
            std::optional<std::uint32_t> position;
            std::optional<std::uint32_t> normal;
            std::optional<std::uint32_t> tangent;
            std::optional<std::uint32_t> texcoord;
        };

        enum class Mode : std::uint32_t
        {
            Points = 0,
            Lines = 1,
            LineLoop = 2,
            LineStrip = 3,
            Triangles = 4,
            TriangleStrip = 5,
            TriangleFan = 6,
        };

        Attributes attributes;
        std::optional<std::uint32_t> indices;
        Mode mode;
        std::optional<std::uint32_t> material;
    };

    struct Mesh
    {
        std::vector<Primitive> primitives;
    };

    struct Node
    {
        std::string name;

        glm::mat4 matrix;

        std::vector<std::uint32_t> children;
        std::optional<std::uint32_t> mesh;
        std::optional<std::uint32_t> camera;
    };

    struct Asset
    {
        std::vector<Node> nodes;
        std::vector<Mesh> meshes;
        std::vector<Material> materials;
        std::vector<Texture> textures;
        std::vector<Image> images;
        std::vector<Accessor> accessors;
        std::vector<BufferView> bufferViews;
        std::vector<Buffer> buffers;
        std::vector<Camera> cameras;
    };

}
