#include "ToonShaderEffect.h"

void ToonShaderEffect::Init(unsigned width, unsigned height)
{
    int index = int(_buffers.size());
    _buffers.push_back(new Framebuffer());
    _buffers[index]->AddColorTarget(GL_RGBA8);
    _buffers[index]->Init(width, height);

    index = int(_shaders.size());
    _shaders.push_back(Shader::Create());
    _shaders[index]->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
    _shaders[index]->LoadShaderPartFromFile("shaders/Post/toon_frag.glsl", GL_FRAGMENT_SHADER);
    _shaders[index]->Link();

    PostEffect::Init(width, height);
}

void ToonShaderEffect::ApplyEffect(PostEffect* buffer)
{
    BindShader(0);
    _shaders[0]->SetUniform("u_Bands", _bands);

    buffer->BindColourAsTexture(0, 0, 0);

    _buffers[0]->RenderToFSQ();

    buffer->UnbindTexture(0);

    UnbindShader();
}

void ToonShaderEffect::DrawToScreen()
{
    BindShader(0);
    _shaders[0]->SetUniform("u_Bands", _bands);
    BindColourAsTexture(0, 0, 0);
    _buffers[0]->DrawFullscreenQuad();
    UnbindTexture(0);
    UnbindShader();
}

int ToonShaderEffect::GetBands()
{
    return _bands;
}

void ToonShaderEffect::SetBands(int bands)
{
    _bands = bands;
}
