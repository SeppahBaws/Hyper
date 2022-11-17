RaytracingAccelerationStructure accel : register(t0);
RWTexture2D<float> image : register(u1);

struct CameraProperties
{
	float4x4 viewInverse;
	float4x4 projInverse;
};
cbuffer camera : register(b2) { CameraProperties camera; };

struct HitInfo
{
	bool hitAnything;
	bool isSecondaryRay;
};

struct Payload
{
	[[vk::location(0)]] HitInfo hitInfo;
};

struct RTPushConstants
{
	float3 sunDir;
	uint frameNr;
};
[[vk::push_constant]] RTPushConstants pushConstants;

// Generates a seed for a random number generator from 2 inputs plus a backoff
// credit: Chris Wyman, from tutorial: http://cwyman.org/code/dxrTutors/tutors/Tutor5/tutorial05.md.html
uint InitRand(uint val0, uint val1, uint backoff = 16)
{
	uint v0 = val0, v1 = val1, s0 = 0;

	[unroll]
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
float3 GetRandomOnUnitSphere(inout uint randSeed)
{
	const float theta = 2 * 3.14159265f * NextRand(randSeed);
	const float phi = acos(1 - 2 * NextRand(randSeed));
	const float x = sin(phi) * cos(theta);
	const float y = sin(phi) * sin(theta);
	const float z = cos(phi);
	return float3(x, y, z);
}

[shader("closesthit")]
void main(inout Payload payload)
{
	if (payload.hitInfo.isSecondaryRay)
	{
		payload.hitInfo.hitAnything = true;
		return;
	}

	// TODO: realistic sun shadow "size"
	// Sun size in the sky (from earth) is 0.53°

	uint randSeed = InitRand(DispatchRaysIndex().x + DispatchRaysIndex().y * DispatchRaysDimensions().x, pushConstants.frameNr);

	const float3 origin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
	const float3 randomOffset = GetRandomOnUnitSphere(randSeed);
	// float sphereOffset = 1.0;
	// float3 direction = normalize(normalize(pushConstants.sunDir) * sphereOffset + randomOffset);
	const float3 direction = normalize(normalize(pushConstants.sunDir) * 10.0 + randomOffset * 0.1);

	RayDesc rayDesc;
	rayDesc.Origin = origin;
	rayDesc.Direction = direction;
	rayDesc.TMin = 0.001;
	rayDesc.TMax = 10000.0;
	payload.hitInfo.isSecondaryRay = true;
	TraceRay(accel, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 0, 0, rayDesc, payload);
}