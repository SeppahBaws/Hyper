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

[shader("raygeneration")]
void main()
{
	const uint3 launchId = DispatchRaysIndex();
	const uint3 launchSize = DispatchRaysDimensions();

	const float2 pixelCenter = launchId.xy + float2(0.5, 0.5);
	const float2 inUV = pixelCenter / launchSize.xy;
	const float2 d = inUV * 2.0 - 1.0;

	const float4 origin = mul(camera.viewInverse, float4(0, 0, 0, 1));
	const float4 target = mul(camera.projInverse, float4(d.x, d.y, 1, 1));
	const float4 direction = mul(camera.viewInverse, float4(normalize(target.xyz), 0));

	Payload payload = (Payload)0;
	payload.hitInfo.hitAnything = false;
	payload.hitInfo.isSecondaryRay = false;

	RayDesc rayDesc;
	rayDesc.Origin = origin.xyz;
	rayDesc.Direction = direction.xyz;
	rayDesc.TMin = 0.001;
	rayDesc.TMax = 10000.0;
	TraceRay(accel, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 0, 0, rayDesc, payload);

	image[int2(launchId.xy)] = float(!payload.hitInfo.hitAnything) * 1.0;
}