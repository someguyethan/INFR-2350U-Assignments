#version 420

layout(binding = 0) uniform sampler2D s_Tex; //Source image
uniform float u_Threshold;

out vec4 frag_color;

in vec2 inUV;

void main() 
{
	vec4 colour = texture(s_Tex, inUV);
	
	float luminance = (colour.r + colour.g + colour.b) / 3.0;
	
	if (luminance > u_Threshold) 
	{
		frag_color = colour;
	}
	else
	{
		frag_color = vec4(0.0, 0.0, 0.0, 1.0);
	}
}