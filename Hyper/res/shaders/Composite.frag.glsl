#version 460 core

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform sampler2D geometryOutput;
layout(set = 0, binding = 1) uniform sampler2D raytraceOutput;

void main()
{
    vec3 geometry = texture(geometryOutput, inUV).rgb;
    float raytrace = clamp(0.15 + texture(raytraceOutput, inUV).r, 0, 1);

    vec3 color = geometry * raytrace;

    // TODO: exposure etc.

    // Gamma correction
    float gamma = 2.2;
    color = pow(color, vec3(1.0 / gamma));

    fragColor = vec4(color, 1.0);
}
