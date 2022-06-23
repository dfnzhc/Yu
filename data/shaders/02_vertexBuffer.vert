#version 460

layout(location = 0) out vec3 fragColor;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout (binding = 0) uniform UBO 
{
	mat4 viewMatrix;
	mat4 projectionMatrix;
} ubo;

void main() {
    gl_Position = ubo.projectionMatrix * ubo.viewMatrix * vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}