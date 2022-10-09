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
hitAttributeEXT vec3 attribs;

layout(push_constant) uniform constants
{
    vec3 sunDir;
    uint frameNr;
} RTPushConstants;

// Generates a seed for a random number generator from 2 inputs plus a backoff
// credit: Chris Wyman, from tutorial: http://cwyman.org/code/dxrTutors/tutors/Tutor5/tutorial05.md.html
uint InitRand(uint val0, uint val1, uint backoff)
{
	uint v0 = val0, v1 = val1, s0 = 0;

	for (uint n = 0; n < backoff; n++)
	{
		s0 += 0x9e3779b9;
		v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
		v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
	}
	return v0;
}

// Takes our seed, updates it, and returns a pseudorandom float in [0..1]
// credit: Chris Wyman, from tutorial: http://cwyman.org/code/dxrTutors/tutors/Tutor5/tutorial05.md.html
float NextRand(inout uint s)
{
	s = (1664525u * s + 1013904223u);
	return float(s & 0x00FFFFFF) / float(0x01000000);
}

// Returns a random direction on the unit sphere, uniformly distributed.
// credit: http://corysimon.github.io/articles/uniformdistn-on-sphere/
vec3 GetRandomOnUnitSphere(inout uint randSeed)
{
	const float theta = 2 * 3.14159265f * NextRand(randSeed);
	const float phi = acos(1 - 2 * NextRand(randSeed));
	const float x = sin(phi) * cos(theta);
	const float y = sin(phi) * sin(theta);
	const float z = cos(phi);
	return vec3(x, y, z);
}

void main()
{
    if (payload.isSecondaryRay)
    {
        payload.rayT = gl_HitTEXT;
        return;
    }
    
    // TODO: realistic sun shadow "size"
    // Sun size in the sky (from earth) is 0.53°

    uint randSeed = InitRand(gl_LaunchIDEXT.x + gl_LaunchIDEXT.y * gl_LaunchIDEXT.x, RTPushConstants.frameNr, 16);
    

    float tmin = 0.001;
    float tmax = 10000.0;
    vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    vec3 randomOffset = GetRandomOnUnitSphere(randSeed);
    float sphereOffset = 1.0;
    // vec3 direction = normalize(normalize(RTPushConstants.sunDir) * sphereOffset + randomOffset);
    vec3 direction = normalize(normalize(RTPushConstants.sunDir) * 10.0 + randomOffset * 0.1);
    payload.isSecondaryRay = true;
    traceRayEXT(accel, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT, 0xFF, 0, 0, 0, origin, tmin, direction, tmax, 0);
}
