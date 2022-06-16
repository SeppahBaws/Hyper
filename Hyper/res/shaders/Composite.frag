#version 460 core

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform sampler2D geometryOutput;
layout(set = 0, binding = 1) uniform sampler2D raytraceOutput;

void main()
{
    vec3 geometry = texture(geometryOutput, inUV).rgb;
    vec3 raytrace = clamp(vec3(0.3) + texture(raytraceOutput, inUV).rgb, 0, 1);
    fragColor = vec4(geometry * raytrace, 1.0);
}
