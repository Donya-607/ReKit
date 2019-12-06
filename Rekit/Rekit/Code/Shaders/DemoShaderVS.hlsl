#include "DemoShader.hlsli"

VS_OUT main( VS_IN vin )
{
	vin.normal.w	= 0;

	float4 norm		= normalize( mul( vin.normal, cbWorld ) );
	float4 light	= normalize( -cbLightDirection );

	float  NL		= saturate( dot( light, norm ) );
	NL				= NL * 0.5f + 0.5f;

	VS_OUT vout;
	vout.pos		= mul( vin.pos, cbWorldViewProjection );
	vout.color		= cbMaterialColor * NL;
	vout.color.a	= cbMaterialColor.a;
	return vout;
}
