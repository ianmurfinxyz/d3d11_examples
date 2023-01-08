struct VS_OUTPUT {
	float4 pos : SV_POSITION;
	float4 color : COLOR;
};

VS_OUTPUT main(float4 inPos : POSITION, float4 inColor : COLOR)
{
    VS_OUTPUT output;
    output.pos = inPos;
    output.color = inColor;
    return output;
}
