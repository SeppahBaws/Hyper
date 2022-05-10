#version 460

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 vUV;

layout(location = 0) out vec3 o_Color;

layout(push_constant) uniform constants
{
    mat4 renderMatrix;
} PushConstants;

void main()
{
    gl_Position = PushConstants.renderMatrix *  vec4(vPosition, 1.0);
    o_Color = vNormal / vec3(2.0) + vec3(0.5);
}
