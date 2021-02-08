#version 420

layout(location = 0) in vec2 inUV;

out vec4 frag_color;

layout (binding = 0) uniform sampler2D s_screenTex;

uniform float u_Intensity = 0.6;

void main() 
{
	vec4 source = texture(s_screenTex, inUV);
	
	vec3 sepiaColour;
	sepiaColour.r = ((source.r * 0.393) + (source.g * 0.769) + (source.b * 0.189));
	sepiaColour.g = ((source.r * 0.349) + (source.g * 0.686) + (source.b * 0.168));
	sepiaColour.b = ((source.r * 0.272) + (source.g * 0.534) + (source.b * 0.131));

	frag_color.rgb = mix(source.rgb, sepiaColour.rgb, u_Intensity);
	frag_color.a = source.a;
}