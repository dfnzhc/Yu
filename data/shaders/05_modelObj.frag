#version 460

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 outNormal;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main() {
    vec3 col = texture(texSampler, uv).xyz;
//    col = pow(col, vec3(1.0 / 2.2));
    
    col *= fragColor;
    
    outColor = vec4(col, 1.0);
}

