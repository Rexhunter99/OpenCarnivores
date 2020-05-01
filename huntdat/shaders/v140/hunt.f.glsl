#version 140

// -- Input/Varying -- //
layout (location=COLOR0) in  flat vec4 o_Color;
layout (location=TEXCOORD0) in  flat vec2 o_TexCoord;

void main()
{
	gl_FragColor = o_Color;
}