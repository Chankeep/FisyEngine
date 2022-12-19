struct Varings
{
	float4 positionCS : SV_POSITION;
	float2 texcoord : TEXCOORD;
};

Texture2D _MainTex : register(t0);
SamplerState sampler_MainTex : register(s0);


float4 main(Varings i) : SV_TARGET
{
	return _MainTex.Sample(sampler_MainTex, i.texcoord);
}