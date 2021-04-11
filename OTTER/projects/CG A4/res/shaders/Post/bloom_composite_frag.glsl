#version 420

layout(binding = 0) uniform sampler2D s_Scene;
layout(binding = 1) uniform sampler2D s_Bloom;

in vec2 inUV;

out vec4 frag_color;

void main() 
{
	vec4 colourA = texture(s_Scene, inUV);
	vec4 colourB = texture(s_Bloom, inUV);

	frag_color = 1.0 - (1.0 - colourA) * (1.0 - colourB);
}