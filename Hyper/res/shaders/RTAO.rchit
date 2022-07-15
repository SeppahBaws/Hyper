#version 460 core
#extension GL_EXT_ray_tracing : require
// #extension GL_EXT_nonuniform_qualifier : enable

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
hitAttributeEXT vec3 attribs;

layout(push_constant) uniform constants
{
    vec3 sunDir;
} LightingSettings;

void main()
{
    if (payload.isSecondaryRay)
    {
        payload.rayT = gl_HitTEXT;
        return;
    }

    float tmin = 0.001;
    float tmax = 10000.0;
    vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    vec3 direction = normalize(LightingSettings.sunDir);
    payload.isSecondaryRay = true;
    traceRayEXT(accel, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT, 0xFF, 0, 0, 0, origin, tmin, direction, tmax, 0);
}
