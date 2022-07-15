#version 460 core

layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec3 vColor;
layout(location = 2) in vec2 vUV;

layout(location = 0) out vec4 fragColor;

layout(set = 1, binding = 0) uniform sampler2D texAlbedo;
layout(set = 1, binding = 1) uniform sampler2D texNormal;

layout(push_constant) uniform constants
{
    // Explicit offset to leave room for vertex shader push constant.
    layout(offset = 64) vec3 sunDir;
} LightingSettings;

void main()
{
    vec3 albedo = texture(texAlbedo, vUV).xyz;

    // TODO: transform the normal to be world-space
    vec3 normal = texture(texNormal, vUV).xyz;

    // Simple HalfLambert diffuse
    float NdotL = max(0.0, dot(normal, normalize(LightingSettings.sunDir)));
    float halfLambert = clamp(NdotL * 0.5 + 0.5, 0, 1);

    vec3 outputColor = albedo * halfLambert;

    fragColor = vec4(outputColor, 1.0);
}
