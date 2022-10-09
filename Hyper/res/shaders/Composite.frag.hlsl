struct PSInput
{
	[[vk::location(0)]] float2 uv : TEXCOORD0;
};

Texture2D textureGeometry : register(t0);
SamplerState samplerGeometry : register(s0);
Texture2D textureRaytrace : register(t1);
SamplerState samplerRaytrace : register(s1);

float4 main(PSInput input) : SV_TARGET
{
	float3 geometry = textureGeometry.Sample(samplerGeometry, input.uv).rgb;
	float raytrace = textureRaytrace.Sample(samplerRaytrace, input.uv).r;

	raytrace = saturate(0.15 + raytrace);

	float3 color = geometry * raytrace;

	float gamma = 1.0 / 2.2;
	color = pow(color, float3(gamma, gamma, gamma));

	return float4(color, 1.0);
}