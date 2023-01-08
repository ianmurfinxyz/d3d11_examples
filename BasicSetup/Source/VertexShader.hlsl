struct VS_OUTPUT {
	float4 pos : SV_POSITION;
	float4 color : COLOR;
};

cbuffer cbPerObject {
	float4x4 WVP;
};

VS_OUTPUT main(float4 inPos : POSITION, float4 inColor : COLOR) {
	VS_OUTPUT output;
	output.pos = mul(inPos, WVP);
	output.color = inColor;
	return output;
}
