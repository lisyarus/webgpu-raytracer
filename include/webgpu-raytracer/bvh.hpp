#pragma once

#include <webgpu-raytracer/aabb.hpp>

struct BVH
{
    struct Node
    {
        AABB aabb;

        // Right child id is always leftChild + 1
        // The top 2 bits of leftChildOrFirstTriangle contain
        //    the splitting axis for ordered traversal
        // Triangle count is > 0 only for leaf nodes

        std::uint32_t leftChildOrFirstTriangle = 0;
        std::uint32_t triangleCount = 0;
    };

    static_assert(sizeof(Node) == 32);

    std::vector<Node> nodes;
    std::vector<std::uint32_t> triangleIDs;
};

BVH buildBVH(std::vector<AABB> const & triangleAABB);
