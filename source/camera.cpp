#include <webgpu-raytracer/camera.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/ext.hpp>

namespace
{

    glm::mat4 glToVkProjection(glm::mat4 matrix)
    {
        // Map from [-1, 1] z-range in OpenGL to [0, 1] z-range in Vulkan, WebGPU, etc
        // Equivalent to doing v.z = (v.z + v.w) / 2 after applying the matrix

        for (int i = 0; i < 4; ++i)
            matrix[i][2] = (matrix[i][2] + matrix[i][3]) / 2.f;

        return matrix;
    }

}

Camera::Camera()
{
    position_ = {0.f, 0.f, 0.f};

    axisX_ = {1.f, 0.f, 0.f};
    axisY_ = {0.f, 1.f, 0.f};
    axisZ_ = {0.f, 0.f, 1.f};

    fovY_ = glm::radians(60.f);
    aspectRatio_ = 1.f;
}

Camera::Camera(glTF::Asset const & asset, glTF::Node const & node)
{
    auto const & camera = asset.cameras[*node.camera];

    position_ = glm::vec3(node.matrix[3]);
    axisX_ = glm::vec3(node.matrix[0]);
    axisY_ = glm::vec3(node.matrix[1]);
    axisZ_ = glm::vec3(node.matrix[2]);

    fovY_ = camera.yFov;
    aspectRatio_ = 1.f;

    zNear_ = camera.zNear;
    zFar_ = camera.zFar.value_or(zNear_ * 1000.f);
}

void Camera::setAspectRatio(float aspectRatio)
{
    aspectRatio_ = aspectRatio;
}

glm::mat4 Camera::viewProjectionMatrix() const
{
    auto projection = glToVkProjection(glm::perspective(fovY_, aspectRatio_, zNear_, zFar_));
    auto rotation = glm::transpose(glm::mat4(glm::mat3(axisX_, axisY_, axisZ_)));
    auto translation = glm::translate(-position_);

    return projection * rotation * translation;
}
