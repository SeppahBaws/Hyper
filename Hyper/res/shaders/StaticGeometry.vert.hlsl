struct VSInput
{
	[[vk::location(0)]] float3 position : POSITION0;
	[[vk::location(1)]] float3 normal : NORMAL0;
	[[vk::location(2)]] float3 tangent : TANGENT0;
	[[vk::location(3)]] float3 binormal : BINORMAL0;
	[[vk::location(4)]] float2 uv : TEXCOORD0;
};

struct VSOutput
{
	float4 position : SV_POSITION;
	[[vk::location(0)]] float3 worldPos : POSITION0;
	[[vk::location(1)]] float3 normal : NORMAL0;
	[[vk::location(2)]] float3 color : COLOR0;
	[[vk::location(3)]] float2 uv : TEXCOORD0;
	[[vk::location(4)]] float3 tangent : TANGENT0;
	[[vk::location(5)]] float3 binormal : BINORMAL0;
};

struct CameraData
{
	float4x4 view;
	float4x4 proj;
	float4x4 viewProj;
};
cbuffer cameraData : register(b0, space0) { CameraData cameraData; }

struct PushConstants
{
	float4x4 modelMatrix;
};

[[vk::push_constant]] PushConstants pushConstants;

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;

	float4x4 transformMatrix = mul(cameraData.viewProj, pushConstants.modelMatrix);
	output.position = mul(transformMatrix, float4(input.position, 1.0));

	output.color = float3(1.0, 1.0, 1.0);
	output.uv = input.uv;
	output.worldPos = mul(float4(input.position, 1.0), pushConstants.modelMatrix).xyz;

	float3x3 M = (float3x3)pushConstants.modelMatrix;
	output.tangent = mul(M, input.tangent);
	output.binormal = mul(M, input.binormal);
	output.normal = mul(M, input.normal);

	return output;
}
