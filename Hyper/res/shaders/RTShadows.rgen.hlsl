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
	float rayT;
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
	const float FLT_MAX = float(0x7F7FFFFF);

	uint3 launchId = DispatchRaysIndex();
	uint3 launchSize = DispatchRaysDimensions();

	const float2 pixelCenter = DispatchRaysIndex().xy + float2(0.5, 0.5);
	const float2 inUV = pixelCenter / DispatchRaysDimensions().xy;
	const float2 d = inUV * 2.0 - 1.0;

	float4 origin = mul(camera.viewInverse, float4(0, 0, 0, 1));
	float4 target = mul(camera.projInverse, float4(d.x, d.y, 1, 1));
	float4 direction = mul(camera.viewInverse, float4(normalize(target.xyz), 0));

	Payload payload = (Payload)0;
	payload.hitInfo.isSecondaryRay = false;

	RayDesc rayDesc;
	rayDesc.Origin = origin.xyz;
	rayDesc.Direction = direction.xyz;
	rayDesc.TMin = 0.001;
	rayDesc.TMax = 10000.0;
	TraceRay(accel, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 0, 0, rayDesc, payload);

	image[int2(launchId.xy)] = float(payload.hitInfo.rayT == FLT_MAX) * 1.0;
}