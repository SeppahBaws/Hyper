#version 460

layout(location = 0) in vec3 i_Position;
layout(location = 1) in vec3 i_Color;

layout(location = 0) out vec3 o_Color;

layout(push_constant) uniform constants
{
    mat4 renderMatrix;
} PushConstants;

void main()
{
    gl_Position = PushConstants.renderMatrix *  vec4(i_Position, 1.0);
    o_Color = i_Color;
}
