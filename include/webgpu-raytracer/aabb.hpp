#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

#include <limits>

struct AABB
{
    glm::vec3 min = glm::vec3( std::numeric_limits<float>::infinity());
    glm::vec3 max = glm::vec3(-std::numeric_limits<float>::infinity());

    void extend(glm::vec3 const & p)
    {
        min = glm::min(min, p);
        max = glm::max(max, p);
    }

    void extend(AABB const & aabb)
    {
        min = glm::min(min, aabb.min);
        max = glm::max(max, aabb.max);
    }

    glm::vec3 diagonal() const
    {
        return max - min;
    }

    glm::vec3 center() const
    {
        return (min + max) / 2.f;
    }

    float surfaceArea() const
    {
        auto d = diagonal();
        return 2.f * (d.x * d.y + d.x * d.z + d.y * d.z);
    }
};
