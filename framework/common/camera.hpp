//
// Created by 秋鱼 on 2022/6/15.
//

#pragma once

#include "math_utils.hpp"

namespace yu {

struct Camera
{
    vec3 eye_pos{};
    vec3 look_target{};

    // 右手系基坐标轴
    Frame frame{};

    mat4 view_mat{};
    mat4 proj_mat{};

    bool flipY = true;

    float aspect_ratio, look_distance;
    float fovV, fovH;
    float zNear, zFar;

    float rot_speed = 0.01f;
    float yaw = 0.0f;
    float pitch = 0.0f;

    void lookAt(const vec3& pos, const vec3& target)
    {
        eye_pos = pos;
        look_target = target;
        look_distance = glm::distance(pos, target);
        view_mat = glm::lookAtLH(pos, target, glm::vec3{0, 1, 0});

        const vec3 direction = glm::normalize(target - pos);
        frame = Frame::FromCameraDirection(direction);

        yaw = glm::atan(-direction.z, direction.x);
        pitch = glm::asin(direction.y);
    }

    void setFov(float fov, uint32_t width, uint32_t height, float nearPlane, float farPlane)
    {
        setFov(fov, static_cast<float>(width) / static_cast<float>(height), nearPlane, farPlane);
    }

    void setFov(float fov, float aspectRatio, float nearPlane, float farPlane)
    {
        aspect_ratio = aspectRatio;

        zNear = nearPlane;
        zFar = farPlane;

        fovH = std::min<float>(fov * aspectRatio, glm::half_pi<float>());
        fovV = fovH / aspectRatio;

        proj_mat = glm::perspective(fovV, aspect_ratio, zNear, zFar);

        if (flipY) {
            proj_mat[1][1] *= -1;
        }
    }

    mat4 getProjViewMat() const
    {
        return proj_mat * view_mat;
    }

    vec3 getRight() const { return {glm::transpose(view_mat) * vec4{frame.x, 0.0}}; }
    vec3 getUp() const { return {glm::transpose(view_mat) * vec4{frame.y, 0.0}}; }
    vec3 getDirection() const { return {glm::transpose(view_mat) * vec4{frame.z, 0.0}}; }

    void updateOrbit(float _yaw, float _pitch)
    {
        yaw = _yaw;
        pitch = _pitch;

        vec3 viewDirLocal = PolarToVector(yaw, pitch);

        vec3 viewDirWorld = frame.FromLocal(viewDirLocal);
        eye_pos = look_target + viewDirLocal * look_distance;

        view_mat = glm::lookAtLH(eye_pos, look_target, glm::vec3{0, 1, 0});
    }

    void zoom(float scrollDir)
    {
        constexpr float zoomStep = 0.5f;
        if (scrollDir > 0) {
            if (look_distance - zoomStep > EPS_F) {
                look_distance -= zoomStep;
            }
        } else if (scrollDir < 0) {
            look_distance += zoomStep;
        }
        auto dir = glm::normalize(eye_pos - look_target);
        eye_pos = look_target + dir * look_distance;

        view_mat = glm::lookAtLH(eye_pos, look_target, glm::vec3{0, 1, 0});
    }
};

} // namespace yu