#version 460 core

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 vColor;
layout(location = 3) in vec2 vUV;
layout(location = 4) in mat3 vTBN;
layout(location = 7) in vec3 vBinormal;
layout(location = 8) in vec3 vTangent;

layout(location = 0) out vec4 fragColor;

layout(set = 1, binding = 0) uniform sampler2D texAlbedo;
layout(set = 1, binding = 1) uniform sampler2D texNormal;

layout(push_constant) uniform constants
{
    // Explicit offset to leave room for vertex shader push constant.
    layout(offset = 64) vec3 sunDir;
} LightingSettings;

// vec3 CalculateNormal(vec3 sampledNormal)
// {
//     if (length(vTangent) == 0.0f)
//         return vNormal;

//     // vec3 binormal = normalize(cross(vNormal, vTangent));
//     // mat3 localAxis = mat3(binormal, vTangent, vNormal);

//     sampledNormal = (2.0f * sampledNormal) - 1.0f;

//     vec3 N = normalize(vNormal * sampledNormal);

//     // N.xy *= -1.0; // x and y were flipped.
//     return N;
// }

// vec3 CalculateNormal(vec3 sampledNormal)
// {
//     if (length(vTangent) == 0.0f)
//         return vNormal;

//     // vec3 binormal = normalize(cross(vNormal, vTangent));
//     // mat3 localAxis = mat3(binormal, vTangent, vNormal);

//     // vec3 newNormal = (2.0f * sampledNormal) - 1.0f;
//     // newNormal = normalize(vTBN * newNormal);

//     // vec3 N = normalize(vNormal * newNormal);
//     // N = normalize(newNormal * )

//     // N.xy *= -1.0; // x and y were flipped.

//     vec3 newNormal = normalize(vTBN * sampledNormal);
//     return newNormal;
// }

// vec3 CalculateNormal(vec3 sampledNormal)
// {
//     vec3 newNormal = vNormal;

//     vec3 binormal = normalize(cross(vTangent, vNormal));
//     mat3x3 localAxis = mat3(binormal, vTangent, vNormal);

//     newNormal = (2.0 * sampledNormal) - 1.0;
//     newNormal = normalize(newNormal * localAxis);

//     return newNormal;
// }

vec3 CalculateWorldNormal()
{
    // vec3 normal = normalize(texture(texNormal, vUV).rgb * 2.0 - 1.0);
    // mat3 TBN = transpose(mat3(vTangent, vBinormal, vNormal));
    // normal = normalize(TBN * normal);
    // return normal;
    // return vNormal;
    // return vTBN * (texture(texNormal, vUV).rgb * 2.0 - 1.0);

    vec3 normal = normalize(texture(texNormal, vUV).rgb * 2.0 - 1.0);
    return normalize(vNormal * normal);
}

void main()
{
    vec3 albedo = texture(texAlbedo, vUV).xyz;

    // // TODO: transform the normal to be world-space
    // vec3 normal = CalculateWorldNormal();

    // // Simple HalfLambert diffuse
    // float NdotL = max(0.0, dot(normal, normalize(LightingSettings.sunDir)));
    // float halfLambert = clamp(NdotL * 0.5 + 0.5, 0, 1);

    // vec3 outputColor = albedo * halfLambert;

    // fragColor = vec4(outputColor, 1.0);

    // vec3 normal = vNormal / 2.0 + 0.5;
    // vec3 normal = CalculateNormal(texture(texNormal, vUV).xyz);
    // normal = normal / 2.0 + 0.5;
    // vec3 normal = texture(texNormal, vUV).xyz;
    // normal = 2.0 * normal - 1;
    // fragColor = vec4(normal, 1);

    // fragColor = vec4(vTangent, 1.0);

    // vec3 normal = vNormal;
    // vec3 tangent = vTangent;
    // vec3 bitangent = cross(vNormal, vTangent);

    // mat3 TBN = mat3(tangent, bitangent, normal);

    // vec3 worldNormal = normalize(TBN * texture(texNormal, vUV).xyz);
    // vec3 worldNormal = CalculateNormal(texture(texNormal, vUV).xyz);
    // vec3 worldNormal = texture(texNormal, vUV).xyz;

    // vec3 worldNormal = CalculateWorldNormal();
    vec3 worldNormal = CalculateWorldNormal();
    // vec3 sampledNormal = texture(texNormal, vUV).xyz;
    // worldNormal *= vTBN * sampledNormal;
    // worldNormal = worldNormal * 0.5 + 0.
    
    float NdotL = max(0.0, dot(worldNormal, normalize(LightingSettings.sunDir)));
    float halfLambert = clamp(NdotL * 0.5 + 0.5, 0, 1);
    // worldNormal = vTBN * sampledNormal;

    
    fragColor = vec4(albedo * halfLambert, 1.0);
    // fragColor = vec4(vec3(halfLambert), 1.0);
    // fragColor = vec4(worldNormal, 1.0);
}
