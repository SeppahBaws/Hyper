#version 460 core

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform sampler2D geometryOutput;
layout(set = 0, binding = 1) uniform sampler2D raytraceOutput;

void main()
{
    vec3 geometry = texture(geometryOutput, inUV).rgb;
    float raytrace = clamp(0.6 + texture(raytraceOutput, inUV).r, 0, 10);

    // Gamma correction
    float gamma = 2.2;
    geometry = pow(geometry, vec3(1.0 / gamma));

    vec3 color = geometry * raytrace;

    // TODO: exposure etc.

    fragColor = vec4(color, 1.0);
}
