#version 460

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 outUV;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;

layout (binding = 0) uniform UBO 
{
	mat4 viewMatrix;
	mat4 projectionMatrix;
} ubo;

void main() {
    gl_Position = ubo.projectionMatrix * ubo.viewMatrix * vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
    outUV = vec2(inUV.x, 1.0 - inUV.y);
}