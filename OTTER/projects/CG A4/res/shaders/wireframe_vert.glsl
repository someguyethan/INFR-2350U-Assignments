//https://www.geeks3d.com/hacklab/20180514/demo-wireframe-shader-opengl-3-2-and-opengl-es-3-1/
#version 150
in vec4 gxl3d_Position;
void main()
{
  gl_Position = gxl3d_Position;
}