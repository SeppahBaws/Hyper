#version 460

layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec3 vColor;

layout(location = 0) out vec4 fragColor;

const vec3 gLightDir = vec3(0.577, -0.577, 0.577);

void main()
{
    float diffuseValue = dot(-vNormal, normalize(gLightDir));
    float halfLambert = diffuseValue * 0.5 + 0.5;
    fragColor = vec4(vColor * halfLambert, 1.0);
    //fragColor = vec4(vNormal, 1.0);
}
