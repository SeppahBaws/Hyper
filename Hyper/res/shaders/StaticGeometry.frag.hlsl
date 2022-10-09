struct PSInput
{
	[[vk::location(0)]] float3 worldPos : POSITION0;
	[[vk::location(1)]] float3 normal : NORMAL0;
	[[vk::location(2)]] float3 color : COLOR0;
	[[vk::location(3)]] float2 uv : TEXCOORD0;
	[[vk::location(4)]] float3 tangent : TANGENT0;
	[[vk::location(5)]] float3 binormal : BINORMAL0;
};

Texture2D textureAlbedo : register(t0, space1);
SamplerState samplerAlbedo : register(s0, space1);
Texture2D textureNormal : register(t1, space1);
SamplerState samplerNormal : register(s1, space1);

struct LightingSettings
{
	// Explicit offset to leave room for vertex shader push constant.
	[[vk::offset(64)]] float3 sunDir;
};

[[vk::push_constant]] LightingSettings lightingSettings;


float3 CalculateWorldNormal(PSInput input)
{
	float3 tangentNormal = normalize(textureNormal.Sample(samplerNormal, input.uv).rgb * 2.0 - 1.0);

	float3 T = normalize(input.tangent);
	float3 B = normalize(input.binormal);
	float3 N = normalize(input.normal);
	float3x3 TBN = float3x3(T, B, N);

	return normalize(mul(TBN, tangentNormal));
}

float4 main(PSInput input) : SV_TARGET
{
	float4 sampledAlbedo = textureAlbedo.Sample(samplerAlbedo, input.uv);
	float3 albedo = sampledAlbedo.rgb;
	float alpha = sampledAlbedo.a;
	
	// albedo *= input.color;

	if (alpha < 0.8)
	{
		discard;
	}

	float3 normal = CalculateWorldNormal(input);

	// Simple HalfLambert diffuse
	float NdotL = max(0.0, dot(normal, normalize(lightingSettings.sunDir)));
	float halfLambert = saturate(NdotL * 0.5 + 0.5);

	// float3 pixelColor = albedo * halfLambert;
	float3 pixelColor = albedo;
	return float4(pixelColor, 1.0);
}
