struct VS_IN
{
	float4 pos		: POSITION;
	float4 normal	: NORMAL;
};
struct VS_OUT
{
	float4 pos		: SV_POSITION;
	float4 color	: COLOR;
};
cbuffer CBufferDemo : register( b0 )
{
	row_major float4x4	cbWorldViewProjection;
	row_major float4x4	cbWorld;
	float4				cbLightDirection;
	float4				cbLightColor;
	float4				cbMaterialColor;
};
