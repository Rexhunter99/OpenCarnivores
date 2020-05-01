#version 140

// -- Input/Attributes -- //
layout (location = POSITION) in vec3 i_Position;
layout (location = COLOR0) in vec4 i_Color;
layout (location = TEXCOORD0) in vec2 i_TexCoord;

layout (std140) uniform matrices
{
    mat4 projection;
    mat4 view;
    mat4 model;
};

// -- Output/Varying -- //
layout (location=COLOR0) out  flat vec4 o_Color;
layout (location=TEXCOORD0) out  flat vec2 o_TexCoord;

void main()
{
	gl_Position = projection * view * model * vec4(i_Position,1.0);
	o_Color = i_Color;
	o_TexCoord = i_TexCoord;
}