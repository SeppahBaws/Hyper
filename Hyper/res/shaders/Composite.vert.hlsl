struct VSOutput
{
	float4 position : SV_POSITION;
	[[vk::location(0)]] float2 uv : TEXCOORD0;
};

VSOutput main(uint vertexIndex : SV_VertexID)
{
	VSOutput output = (VSOutput)0;
	output.uv = float2((vertexIndex << 1) & 2, vertexIndex & 2);
	output.position = float4(output.uv * 2.0 - 1.0, 1.0, 1.0);
	return output;
}