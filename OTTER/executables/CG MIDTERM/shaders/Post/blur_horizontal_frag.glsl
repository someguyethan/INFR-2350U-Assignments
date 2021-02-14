#version 420

layout(binding = 0) uniform sampler2D s_Tex; //Source image
uniform float u_PixelSize; //1.0 / Window_Height

out vec4 frag_color;

in vec2 inUV;

void main() 
{
	//Sample pixels in a horizontal row
	//Weight should add up to 1
	frag_color = vec4(0.0, 0.0, 0.0, 0.0);

	frag_color += texture(s_Tex, vec2(inUV.x - 4.0 * u_PixelSize, inUV.y)) * 0.06;
	frag_color += texture(s_Tex, vec2(inUV.x - 3.0 * u_PixelSize, inUV.y)) * 0.09;
	frag_color += texture(s_Tex, vec2(inUV.x - 2.0 * u_PixelSize, inUV.y)) * 0.12;
	frag_color += texture(s_Tex, vec2(inUV.x - 		 u_PixelSize, inUV.y)) * 0.15;
	frag_color += texture(s_Tex, vec2(inUV.x				    , inUV.y)) * 0.16;
	frag_color += texture(s_Tex, vec2(inUV.x +		 u_PixelSize, inUV.y)) * 0.15;
	frag_color += texture(s_Tex, vec2(inUV.x + 2.0 * u_PixelSize, inUV.y)) * 0.12;
	frag_color += texture(s_Tex, vec2(inUV.x + 3.0 * u_PixelSize, inUV.y)) * 0.09;
	frag_color += texture(s_Tex, vec2(inUV.x + 4.0 * u_PixelSize, inUV.y)) * 0.06;
}