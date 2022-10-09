#version 460 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBinormal;
layout(location = 4) in vec2 inUV;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outColor;
layout(location = 3) out vec2 outUV;
layout(location = 4) out vec3 outTangent;
layout(location = 5) out vec3 outBinormal;
layout(location = 6) out mat3 outTBN;

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
    gl_Position = transformMatrix * vec4(inPosition, 1.0);

    outColor = vec3(1.0);
    outUV = inUV;
    outWorldPos = vec3(PushConstants.modelMatrix * vec4(inPosition, 1.0));

    mat3 M = mat3(PushConstants.modelMatrix);
    vec3 N = inNormal;
    vec3 T = inTangent;
    vec3 B = inBinormal;
    outTBN = M * mat3(T, B, N);

    outTangent = M * T;
    outBinormal = M * B;
    outNormal = M * N;
}
