#version 420

layout(location = 0) in vec2 inUV;

out vec4 frag_color;

layout (binding = 0) uniform sampler2D s_screenTex;

uniform float amount = 0.1;

float random( vec2 p )
{
  vec2 K1 = vec2(
    23.14069263277926, // e^pi (Gelfond's constant)
    2.665144142690225 // 2^sqrt(2) (Gelfond–Schneider constant)
  );
return fract( cos( dot(p,K1) ) * 12345.6789 );
}

void main() {
  vec4 color = texture2D( s_screenTex, inUV );
  vec2 uvRandom = inUV;
  uvRandom.y *= random(vec2(uvRandom.y,amount));
  color.rgb += random(uvRandom)*0.15;
  frag_color = vec4(color);
}
//Adapted from https://simonharris.co/making-a-noise-film-grain-post-processing-effect-from-scratch-in-threejs/