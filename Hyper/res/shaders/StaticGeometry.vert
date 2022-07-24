#version 460 core

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 vTangent;
layout(location = 3) in vec3 vBinormal;
layout(location = 4) in vec2 vUV;

layout(location = 0) out vec3 o_WorldPos;
layout(location = 1) out vec3 o_Normal;
layout(location = 2) out vec3 o_Color;
layout(location = 3) out vec2 o_UV;
layout(location = 4) out mat3 o_TBN;
layout(location = 7) out vec3 o_Binormal;
layout(location = 8) out vec3 o_Tangent;

layout(set = 0, binding = 0) uniform CameraData
{
    mat4 view;
    mat4 proj;
    mat4 viewProj;
} cameraData;

layout(push_constant) uniform constants
{
    mat4 modelMatrix;
} PushConstants;

void main()
{
    mat4 transformMatrix = cameraData.viewProj * PushConstants.modelMatrix;
    gl_Position = transformMatrix * vec4(vPosition, 1.0);

    vec3 worldNormal = normalize(mat3(PushConstants.modelMatrix) * vNormal);
    vec3 worldTangent = normalize(mat3(PushConstants.modelMatrix) * vTangent);
    vec3 worldBinormal = normalize(mat3(PushConstants.modelMatrix) * vBinormal);

    o_WorldPos = mat3(PushConstants.modelMatrix) * vPosition;
    o_Color = vec3(1.0);
    o_UV = vUV;

    o_Normal = worldNormal;
    o_Binormal = worldTangent;
    o_Tangent = worldBinormal;

    o_TBN = transpose(mat3(worldTangent, worldBinormal, worldNormal));
}
