#version 420

layout(location = 0) in vec2 inUV;

layout (binding = 0) uniform sampler2D s_screenTex;

out vec4 frag_color;

uniform int u_Bands;

const float scaleFactor = 1.0/u_Bands;

float toonify(in float intensity) {
    if (intensity > 0.8)
        return 1.0;
    else if (intensity > 0.5)
        return 0.8;
    else if (intensity > 0.25)
        return 0.3;
    else
        return 0.1;
}

void main() 
{
	vec4 source = texture(s_screenTex, inUV);
    float factor = toonify(max(source.r, max(source.g, source.b)));
	frag_color.rgb = factor*source.rgb;
	frag_color.a = source.a;
}