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
layout(location = 0) rayPayloadEXT HitInfo payload;

layout(push_constant) uniform constants
{
    vec3 sunDir;
    uint frameNr;
} RTPushConstants;

const float FLT_MAX = float(0x7F7FFFFF);

void main()
{
    const vec2 pixelCenter = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
    const vec2 inUV = pixelCenter / vec2(gl_LaunchSizeEXT.xy);
    vec2 d = inUV * 2.0 - 1.0;

    vec4 origin = camera.viewInverse * vec4(0, 0, 0, 1);
    vec4 target = camera.projInverse * vec4(d.x, d.y, 1, 1);
    vec4 direction = camera.viewInverse * vec4(normalize(target.xyz), 0);

    float tmin = 0.001;
    float tmax = 10000.0;

    payload.isSecondaryRay = false;

    traceRayEXT(accel, gl_RayFlagsOpaqueEXT, 0xFF, 0, 0, 0, origin.xyz, tmin, direction.xyz, tmax, 0);

    float outputColor = float(payload.rayT == FLT_MAX) * 1.0;
    imageStore(image, ivec2(gl_LaunchIDEXT.xy), vec4(outputColor));
}
