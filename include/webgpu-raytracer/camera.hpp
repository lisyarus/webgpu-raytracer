#pragma once

#include <webgpu-raytracer/gltf_asset.hpp>

#include <glm/glm.hpp>

struct Camera
{
    Camera();
    Camera(Camera const &) = default;
    Camera(glTF::Asset const & asset, glTF::Node const & node);

    void setAspectRatio(float aspectRatio);

    void rotateX(float angle);
    void rotateY(float angle);
    void rotateZ(float angle);

    void moveForward(float distance);
    void moveRight(float distance);

    glm::mat4 viewProjectionMatrix() const;

private:
    glm::vec3 position_;

    glm::vec3 axisX_;
    glm::vec3 axisY_;
    glm::vec3 axisZ_;

    float fovY_;
    float aspectRatio_;
    float zNear_;
    float zFar_;
};
