#pragma once

#include <webgpu-raytracer/aabb.hpp>

struct BVH
{
    struct Node
    {
        // Right child id is always leftChild + 1
        // The top 2 bits of leftChildOrFirstTriangle contain
        //    the splitting axis for ordered traversal
        // Triangle count is > 0 only for leaf nodes

        glm::vec3 aabbMin{std::numeric_limits<float>::infinity()};
        std::uint32_t leftChildOrFirstTriangle = 0;
        glm::vec3 aabbMax{-std::numeric_limits<float>::infinity()};
        std::uint32_t triangleCount = 0;
    };

    static_assert(sizeof(Node) == 32);

    std::vector<Node> nodes;
    std::vector<std::uint32_t> triangleIDs;
};

BVH buildBVH(std::vector<AABB> const & triangleAABB);
