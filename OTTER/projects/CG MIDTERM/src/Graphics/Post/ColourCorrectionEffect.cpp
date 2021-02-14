#include "ColourCorrectionEffect.h"

void ColourCorrectionEffect::Init(unsigned width, unsigned height)
{
    int index = int(_buffers.size());
    _buffers.push_back(new Framebuffer());
    _buffers[index]->AddColorTarget(GL_RGBA8);
    _buffers[index]->AddDepthTarget();
    _buffers[index]->Init(width, height);

    index = int(_shaders.size());
    _shaders.push_back(Shader::Create());
    _shaders[index]->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
    _shaders[index]->LoadShaderPartFromFile("shaders/Post/colour_correction_frag.glsl", GL_FRAGMENT_SHADER);
    _shaders[index]->Link();
}

void ColourCorrectionEffect::ApplyEffect(PostEffect* buffer)
{
    BindShader(0);
    //_shaders[0]->SetUniform("u_TexColourGrade", _LUT);

    buffer->BindColourAsTexture(0, 0, 0);

    _LUT.bind(30);

    _buffers[0]->RenderToFSQ();

    _LUT.unbind(30);

    buffer->UnbindTexture(0);

    UnbindShader();
}

void ColourCorrectionEffect::DrawToScreen()
{
    BindShader(0);
    //_shaders[0]->SetUniform("u_TexColourGrade", _LUT);
    BindColourAsTexture(0, 0, 0);
    _buffers[0]->DrawFullscreenQuad();
    UnbindTexture(0);
    UnbindShader();
}

void ColourCorrectionEffect::SetLUT(LUT3D lut)
{
    _LUT = lut;
}

LUT3D ColourCorrectionEffect::GetLUT()
{
    return _LUT;
}
