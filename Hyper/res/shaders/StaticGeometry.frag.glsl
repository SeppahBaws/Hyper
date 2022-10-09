#version 460 core

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inUV;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBinormal;
layout(location = 6) in mat3 inTBN;

layout(location = 0) out vec4 fragColor;

layout(set = 1, binding = 0) uniform sampler2D texAlbedo;
layout(set = 1, binding = 1) uniform sampler2D texNormal;

layout(push_constant) uniform constants
{
    // Explicit offset to leave room for vertex shader push constant.
    layout(offset = 64) vec3 sunDir;
} LightingSettings;

vec3 CalculateWorldNormal()
{
    vec3 sampledNormal = texture(texNormal, inUV).xyz;
    
    vec3 worldNormal = inTBN * normalize(sampledNormal * 2.0 - 1.0);

    return worldNormal;
}

void main()
{
    vec4 sampledAlbedo = texture(texAlbedo, inUV).rgba;
    vec3 albedo = sampledAlbedo.rgb;
    float alpha = sampledAlbedo.a;

    if (alpha < 0.8)
    {
        discard;
    }

    vec3 normal = CalculateWorldNormal();
    
    // Simple HalfLambert diffuse
    float NdotL = max(0.0, dot(normal, normalize(LightingSettings.sunDir)));
    float halfLambert = clamp(NdotL * 0.5 + 0.5, 0, 1);

    vec3 outputColor = albedo * halfLambert;

    fragColor = vec4(outputColor, 1.0);
}
