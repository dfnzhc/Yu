#version 460

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform TEST
{
    vec3 col; 
}test;

void main() {
    outColor = vec4(test.col, 1.0);
}

