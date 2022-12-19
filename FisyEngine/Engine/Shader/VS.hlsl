
struct Varings
{
	float4 positionCS : SV_POSITION;
	float2 texcoord : TEXCOORD;
};

cbuffer MVPBuffer : register(b0)
{
	float4x4 MVP;
};

Varings main(float4 positionOS : POSITION, float2 texcoord : TEXCOORD)
{
	Varings v;
	v.texcoord = texcoord;
	v.positionCS = mul(positionOS, MVP);
	return v;
}
