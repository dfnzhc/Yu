//
// Created by 秋鱼 on 2022/5/27.
//

#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

using glm::vec3;
using glm::vec4;
using glm::mat4;

namespace ST {

enum class CameraType
{
    EyeFixed, Orbit, FirstPerson
};

struct Camera
{
    float fovY = 60.0f;
    float zNear = 0.01f, zFar = 1000.0f;
    float pitch = 0.0f, yaw = 0.0f;
    float look_distance;
    CameraType type = CameraType::Orbit;

    vec3 rotation{};
    vec3 position{0, 1, 3};
    vec3 target{0, 0, 0};

    vec3 u, v, w;

    float rotation_speed = 0.5f;
    float movement_speed = 1.0f;

    bool flipY = true;

    struct
    {
        mat4 view{};
        mat4 perspective{};
    } matrices;

    void perspectiveInit(float fov, float aspect, float znear, float zfar)
    {
        this->fovY = fov;
        this->zNear = znear;
        this->zFar = zfar;

        matrices.perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
        if (flipY) {
            matrices.perspective[1][1] *= -1.0f;
        }
        look_distance = glm::distance(target, position);

        w = glm::normalize(position - target);
        u = glm::normalize(glm::cross(glm::vec3{0, 1, 0}, w));
        v = glm::normalize(glm::cross(w, u));
        matrices.view = glm::lookAt(position, target, v);

        if (type == CameraType::EyeFixed) {
            pitch = glm::asin(w.y);
            yaw = glm::acos(w.x);
        } else if (type == CameraType::Orbit) {
            pitch = glm::acos(v.y);
            yaw = glm::acos(-w.x);
        }
    }

    void updateAspectRatio(float aspect)
    {
        matrices.perspective = glm::perspective(glm::radians(fovY), aspect, zNear, zFar);
        if (flipY) {
            matrices.perspective[1][1] *= -1.0f;
        }
    }

    void update(float dx, float dy)
    {
        pitch = glm::radians(std::min(89.0f, std::max(-89.0f, glm::degrees(pitch) + rotation_speed * dy)));
        yaw = glm::radians(fmod(glm::degrees(yaw) + rotation_speed * dx, 360.0f));

        vec3 viewDirLocal;
        viewDirLocal.x = glm::cos(pitch) * glm::cos(yaw);
        viewDirLocal.y = glm::sin(pitch);
        viewDirLocal.z = glm::cos(pitch) * glm::sin(yaw);

        vec3 viewDirWorld = u * viewDirLocal.x + v * viewDirLocal.y + w * viewDirLocal.z;
        if (type == CameraType::EyeFixed) {
            target = position - viewDirWorld * look_distance;
        } else if (type == CameraType::Orbit) {
            position = target + viewDirWorld * look_distance;
        }

        matrices.view = glm::lookAt(position, target, v);
    }
};

} // namespace ST