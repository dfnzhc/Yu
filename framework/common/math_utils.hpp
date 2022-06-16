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



} // namespace yu