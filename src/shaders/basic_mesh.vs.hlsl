struct VS_INPUT
{
	float3 pos : POSITION;
	float3 col : COLOR;
};

struct VSOUPUT
{
	float4 position : SV_Position;
	float4 color : COLOR0;
};

struct BufferAddr
{
	uint64_t addr;
};

[[vk::push_constant]]
ConstantBuffer<BufferAddr> buffer;

VSOUPUT main(VS_INPUT input)
{
	VSOUPUT output = {
		{ input.pos, 1.f },
		{ input.col, 1.f }
	};
	
	return output;
}