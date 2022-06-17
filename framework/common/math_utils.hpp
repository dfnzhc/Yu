//
// Created by 秋鱼 on 2022/6/15.
//

#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

using glm::vec3;
using glm::vec4;
using glm::mat3;
using glm::mat4;

namespace yu {

constexpr double EPS = std::numeric_limits<double>::epsilon();
constexpr float EPS_F = std::numeric_limits<float>::epsilon();

template<typename T>
inline T AlignUp(T val, T alignment)
{
    return (val + alignment - static_cast<T>(1)) & ~(alignment - static_cast<T>(1));
}

/**
 * @brief 根据两个球坐标系下的角度，给出响应的向量；角度 phi 从 X 轴开始增长
 */
inline vec3 PolarToVector(float phi, float theta)
{
    return {std::cosf(phi) * std::cosf(theta), std::sinf(theta), std::sinf(phi) * std::cosf(theta)};
}

/**
 * @brief 根据两个球坐标系下的角度，给出响应的向量；角度 phi 从 Z 轴开始增长
 */
inline vec3 PolarToVectorZAxis(float phi, float theta)
{
    return {std::sinf(phi) * std::cosf(theta), std::sinf(theta), std::cosf(phi) * std::cosf(theta)};
}

inline void CoordinateSystem(const vec3& v1, vec3& v2, vec3& v3)
{
    float sign = std::copysign(float(1), v1.z);
    float a = -1 / (sign + v1.z);
    float b = v1.x * v1.y * a;
    v2 = vec3{1 + sign * std::sqrt(v1.x) * a, sign * b, -sign * v1.x};
    v3 = vec3{b, sign + std::sqrt(v1.y) * a, -v1.y};
}

// Frame Definition
struct Frame
{
    Frame() : x(1, 0, 0), y(0, 1, 0), z(0, 0, 1) {}

    Frame(vec3 x, vec3 y, vec3 z) : x{x}, y{y}, z{z} {}

    static Frame FromXZ(vec3 x, vec3 z) { return {x, glm::cross(z, x), z}; }

    static Frame FromXY(vec3 x, vec3 y) { return {x, y, glm::cross(x, y)}; }

    static Frame FromZ(vec3 z)
    {
        vec3 x, y;
        CoordinateSystem(z, x, y);
        return {x, y, z};
    }

    static Frame FromX(vec3 x)
    {
        vec3 y, z;
        CoordinateSystem(x, y, z);
        return {x, y, z};
    }

    static Frame FromY(vec3 y)
    {
        vec3 x, z;
        CoordinateSystem(y, z, x);
        return {x, y, z};
    }

    static Frame FromCameraDirection(vec3 dir)
    {
        const vec3 worldUp{0, 1, 0};
        vec3 right = glm::normalize(glm::cross(glm::normalize(worldUp), dir));
        return FromXZ(right, dir);
    }

    vec3 ToLocal(vec3 v) const
    {
        return {glm::dot(v, x), glm::dot(v, y), glm::dot(v, z)};
    }

    vec3 FromLocal(vec3 v) const { return v.x * x + v.y * y + v.z * z; }

    // Frame Public Members
    vec3 x, y, z;
};

} // namespace yu