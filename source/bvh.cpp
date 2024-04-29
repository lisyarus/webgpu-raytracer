#include <webgpu-raytracer/bvh.hpp>

#include <algorithm>
#include <iostream>

namespace
{

    constexpr std::uint32_t MAX_DEPTH = 32;

    template <typename Iterator>
    void buildNode(BVH & bvh, std::vector<AABB> const & triangleAABB, std::uint32_t nodeID, Iterator trianglesBegin, Iterator trianglesEnd, std::uint32_t depth, std::uint32_t & maxDepth)
    {
        maxDepth = std::max(depth, maxDepth);

        auto & node = bvh.nodes[nodeID];
        for (auto it = trianglesBegin; it != trianglesEnd; ++it)
            node.aabb.extend(triangleAABB[*it]);

        std::uint32_t const triangleCount = trianglesEnd - trianglesBegin;

        // Greedy surface-area heuristic: try all possible subdivisions into 2 child nodes
        // over all 3 possible axes to select the most optimal one

        float nodeSelfCost = triangleCount * node.aabb.surfaceArea();

        std::uint32_t bestSplitAxis = 0;
        std::uint32_t bestSplitPosition = 0;
        float bestSplitCost = std::numeric_limits<float>::infinity();

        for (std::uint32_t axis = 0; axis < 3; ++axis)
        {
            std::sort(trianglesBegin, trianglesEnd, [&](std::uint32_t triangle1, std::uint32_t triangle2){
                return triangleAABB[triangle1].center()[axis] < triangleAABB[triangle2].center()[axis];
            });

            std::vector<AABB> leftToRightAABB;
            std::vector<AABB> rightToLeftAABB;

            leftToRightAABB.push_back(AABB{});
            for (auto it = trianglesBegin; it != trianglesEnd; ++it)
            {
                leftToRightAABB.push_back(leftToRightAABB.back());
                leftToRightAABB.back().extend(triangleAABB[*it]);
            }

            rightToLeftAABB.push_back(AABB{});
            for (auto it = std::make_reverse_iterator(trianglesEnd); it != std::make_reverse_iterator(trianglesBegin); ++it)
            {
                rightToLeftAABB.push_back(rightToLeftAABB.back());
                rightToLeftAABB.back().extend(triangleAABB[*it]);
            }

            for (std::uint32_t i = 1; i + 1 < triangleCount; ++i)
            {
                auto leftAABB = leftToRightAABB[i];
                auto rightAABB = rightToLeftAABB[triangleCount - i];
                std::uint32_t leftCount = i;
                std::uint32_t rightCount = triangleCount - i;

                float cost = leftAABB.surfaceArea() * leftCount + rightAABB.surfaceArea() * rightCount;

                if (cost < bestSplitCost)
                {
                    bestSplitCost = cost;
                    bestSplitAxis = axis;
                    bestSplitPosition = i;
                }
            }
        }

        if (triangleCount <= 4 || nodeSelfCost < bestSplitCost || depth == MAX_DEPTH)
        {
            // Create leaf node
            node.leftChildOrFirstTriangle = trianglesBegin - bvh.triangleIDs.begin();
            node.triangleCount = triangleCount;
        }
        else
        {
            // Split into 2 child nodes

            std::uint32_t leftChild = bvh.nodes.size();

            node.leftChildOrFirstTriangle = leftChild | (bestSplitAxis << 30);
            node.triangleCount = 0;

            bvh.nodes.emplace_back();
            bvh.nodes.emplace_back();

            // NB: can't access `node` reference here, because emplace_back() could've
            // reallocated the underlying array

            std::sort(trianglesBegin, trianglesEnd, [&](std::uint32_t triangle1, std::uint32_t triangle2){
                return triangleAABB[triangle1].center()[bestSplitAxis] < triangleAABB[triangle2].center()[bestSplitAxis];
            });

            auto splitIt = trianglesBegin + bestSplitPosition;

            buildNode(bvh, triangleAABB, leftChild, trianglesBegin, splitIt, depth + 1, maxDepth);
            buildNode(bvh, triangleAABB, leftChild + 1, splitIt, trianglesEnd, depth + 1, maxDepth);
        }
    }

}

BVH buildBVH(std::vector<AABB> const & triangleAABB)
{
    BVH result;
    result.triangleIDs.resize(triangleAABB.size());
    for (std::uint32_t i = 0; i < result.triangleIDs.size(); ++i)
        result.triangleIDs[i] = i;

    std::uint32_t maxDepth = 0;

    result.nodes.emplace_back();
    buildNode(result, triangleAABB, 0, result.triangleIDs.begin(), result.triangleIDs.end(), 0, maxDepth);

    std::cout << "BVH max depth: " << maxDepth << std::endl;

    return result;
}
