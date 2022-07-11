#version 460

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 outUV;
layout(location = 2) out vec3 outNormal;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec3 inColor;

layout (binding = 0) uniform UBO 
{
	mat4 viewMatrix;
	mat4 projectionMatrix;
} ubo;

void main() {
    gl_Position = ubo.projectionMatrix * ubo.viewMatrix * vec4(inPosition, 1.0);
    fragColor = inColor;
    outNormal = inNormal;
    outUV = vec2(inUV.x, 1.0 - inUV.y);
}