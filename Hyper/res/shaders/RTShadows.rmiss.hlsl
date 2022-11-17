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

[shader("miss")]
void main(inout Payload payload)
{
	payload.hitInfo.hitAnything = false;
}