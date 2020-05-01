struct VOut
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD;
	float4 col : COLOR;
};

VOut VertexMain(float4 pos : POSITION, float2 tex : TEXCOORD, uint4 col : COLOR)
{
	VOut output;

	output.pos = pos;
	output.tex = tex;
	output.col = col;

	return output;
}

float4 PixelMain(float4 position : SV_POSITION, float2 uv : TEXCOORD, uint4 color : COLOR) : SV_TARGET
{
	//Texture2D<uint4> gDiffuseMap;
	return color;
}