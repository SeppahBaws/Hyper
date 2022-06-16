#version 460 core
#extension GL_EXT_ray_tracing : require

layout(binding = 0, set = 0) uniform accelerationStructureEXT accel;
layout(binding = 1, set = 0, rgba32f) uniform image2D image;
layout(binding = 2, set = 0) uniform CameraProperties
{
    mat4 viewInverse;
    mat4 projInverse;
} camera;

struct HitInfo
{
    vec3 color;
    bool isSecondaryRay;
    float rayT;
};
layout(location = 0) rayPayloadInEXT HitInfo payload;

const float FLT_MAX = float(0x7F7FFFFF);

void main()
{
    payload.color = vec3(0.0);
    payload.rayT = FLT_MAX;
}
