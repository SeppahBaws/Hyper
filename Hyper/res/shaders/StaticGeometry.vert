#version 460

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vUV;

layout(location = 0) out vec3 o_Normal;
layout(location = 1) out vec3 o_Color;
layout(location = 2) out vec2 o_UV;

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
    o_Normal = vNormal;
    o_Color = vec3(1.0);
    o_UV = vUV;
}
