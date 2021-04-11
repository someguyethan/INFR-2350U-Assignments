#include "GBuffer.h"

void GBuffer::Init(unsigned width, unsigned height)
{
	_windowWidth = width;
	_windowHeight = height;

	_gBuffer.AddColorTarget(GL_RGBA8);//Albedo
	_gBuffer.AddColorTarget(GL_RGB8);//Normal
	_gBuffer.AddColorTarget(GL_RGB8);//Specular
	_gBuffer.AddColorTarget(GL_RGB32F);//Position
	_gBuffer.AddDepthTarget();//Depth

	_gBuffer.Init(width, height);

	_passThrough = Shader::Create();
	_passThrough->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
	_passThrough->LoadShaderPartFromFile("shaders/passthrough_frag.glsl", GL_FRAGMENT_SHADER);
	_passThrough->Link();
}

void GBuffer::Bind()
{
	_gBuffer.Bind();
}

void GBuffer::BindLighting()
{
	_gBuffer.BindColorAsTexture(Target::ALBEDO, 0);
	_gBuffer.BindColorAsTexture(Target::NORMAL, 1);
	_gBuffer.BindColorAsTexture(Target::SPECULAR, 2);
	_gBuffer.BindColorAsTexture(Target::POSITION, 3);
}

void GBuffer::Clear()
{
	_gBuffer.Clear();
}

void GBuffer::Unbind()
{
	_gBuffer.Unbind();
}

void GBuffer::UnbindLighting()
{
	_gBuffer.UnbindTexture(0);
	_gBuffer.UnbindTexture(1);
	_gBuffer.UnbindTexture(2);
	_gBuffer.UnbindTexture(3);
}

void GBuffer::DrawBuffersToScreen()
{
	_passThrough->Bind();

	//Albedo top left
	glViewport(0, _windowHeight / 2.0f, _windowWidth / 2.0f, _windowHeight / 2.0f);
	_gBuffer.BindColorAsTexture(Target::ALBEDO, 0);
	_gBuffer.DrawFullscreenQuad();
	_gBuffer.UnbindTexture(0);
	//Normal top right
	glViewport(_windowWidth / 2.0f, _windowHeight / 2.0f, _windowWidth / 2.0f, _windowHeight / 2.0f);
	_gBuffer.BindColorAsTexture(Target::NORMAL, 0);
	_gBuffer.DrawFullscreenQuad();
	_gBuffer.UnbindTexture(0);
	//Specular bottom left
	glViewport(0, 0, _windowWidth / 2.0f, _windowHeight / 2.0f);
	_gBuffer.BindColorAsTexture(Target::SPECULAR, 0);
	_gBuffer.DrawFullscreenQuad();
	_gBuffer.UnbindTexture(0);
	//Position bottom right
	glViewport(_windowWidth / 2.0f, 0, _windowWidth / 2.0f, _windowHeight / 2.0f);
	_gBuffer.BindColorAsTexture(Target::POSITION, 0);
	_gBuffer.DrawFullscreenQuad();
	_gBuffer.UnbindTexture(0);

	_passThrough->UnBind();
}

void GBuffer::Reshape(unsigned width, unsigned height)
{
	_windowHeight = height;
	_windowWidth = width;

	_gBuffer.Reshape(width, height);
}
