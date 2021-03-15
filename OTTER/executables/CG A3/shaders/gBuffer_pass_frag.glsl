#version 420

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec2 inUV;

uniform sampler2D s_Diffuse;
uniform sampler2D s_Diffuse2;
uniform sampler2D s_Specular;
uniform float u_textureMix;

layout(location = 0) out vec4 outColors;
layout(location = 1) out vec3 outNormals;
layout(location = 2) out vec3 outSpecs;
layout(location = 3) out vec3 outPositions;

void main()
{
	vec4 texCol1 = texture(s_Diffuse, inUV);
	vec4 texCol2 = texture(s_Diffuse2, inUV);
	vec4 texCol = mix(texCol1, texCol2, u_textureMix);

	outColors = texCol;

	outNormals = (normalize(inNormal) * 0.5) + 0.5;

	outSpecs = texture(s_Specular, inUV).rgb;

	outPositions = inPos;
}