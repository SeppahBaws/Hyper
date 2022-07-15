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
    float rayT;
    bool isSecondaryRay;
};
layout(location = 0) rayPayloadInEXT HitInfo payload;

layout(push_constant) uniform constants
{
    vec3 sunDir;
} LightingSettings;

const float FLT_MAX = float(0x7F7FFFFF);

void main()
{
    payload.rayT = FLT_MAX;
}
