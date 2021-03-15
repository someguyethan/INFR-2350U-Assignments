#include "BloomEffect.h"

void BloomEffect::Init(unsigned width, unsigned height)
{
    int index = int(_buffers.size());
    _buffers.push_back(new Framebuffer());
    _buffers[index]->AddColorTarget(GL_RGBA8);
    _buffers[index]->Init(width, height);
    index++;
    _buffers.push_back(new Framebuffer());
    _buffers[index]->AddColorTarget(GL_RGBA8);
    _buffers[index]->Init(width, height);
    index++;
    _buffers.push_back(new Framebuffer());
    _buffers[index]->AddColorTarget(GL_RGBA8);
    _buffers[index]->Init(width, height);
    index++;
    _buffers.push_back(new Framebuffer());
    _buffers[index]->AddColorTarget(GL_RGBA8);
    _buffers[index]->Init(width, height);

    index = int(_shaders.size());
    _shaders.push_back(Shader::Create());
    _shaders[index]->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
    _shaders[index]->LoadShaderPartFromFile("shaders/passthrough_frag.glsl", GL_FRAGMENT_SHADER);
    _shaders[index]->Link();
    index++;
    _shaders.push_back(Shader::Create());
    _shaders[index]->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
    _shaders[index]->LoadShaderPartFromFile("shaders/Post/bloom_high_pass_frag.glsl", GL_FRAGMENT_SHADER);
    _shaders[index]->Link();
    index++;
    _shaders.push_back(Shader::Create());
    _shaders[index]->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
    _shaders[index]->LoadShaderPartFromFile("shaders/Post/blur_horizontal_frag.glsl", GL_FRAGMENT_SHADER);
    _shaders[index]->Link();
    index++;
    _shaders.push_back(Shader::Create());
    _shaders[index]->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
    _shaders[index]->LoadShaderPartFromFile("shaders/Post/blur_vertical_frag.glsl", GL_FRAGMENT_SHADER);
    _shaders[index]->Link();
    index++;
    _shaders.push_back(Shader::Create());
    _shaders[index]->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
    _shaders[index]->LoadShaderPartFromFile("shaders/Post/bloom_composite_frag.glsl", GL_FRAGMENT_SHADER);
    _shaders[index]->Link();

    //Pixel size
    _pixelSize = glm::vec2(1.0f / width, 1.0f / height);

    PostEffect::Init(width, height);
}

void BloomEffect::ApplyEffect(PostEffect* buffer)
{
    //Takes in scene as our first buffer
    BindShader(0);

    buffer->BindColorAsTexture(0, 0, 0);

    _buffers[0]->RenderToFSQ();

    buffer->UnbindTexture(0);

    UnbindShader();

    //High pass (find high intensity colours and discard the rest)
    BindShader(1);
    _shaders[1]->SetUniform("u_Threshold", _threshold);

    BindColorAsTexture(0, 0, 0);

    _buffers[1]->RenderToFSQ();

    UnbindTexture(0);

    UnbindShader();

    //Does both a horizontal and vertical blur, an amount of times equal to _passes
    for (unsigned i = 0; i < _passes; i++)
    {
        //Horizontal pass
        BindShader(2);
        _shaders[2]->SetUniform("u_PixelSize", _pixelSize.x);

        BindColorAsTexture(1, 0, 0);

        _buffers[2]->RenderToFSQ();

        UnbindTexture(0);

        UnbindShader();

        //Vertical pass
        BindShader(3);
        _shaders[3]->SetUniform("u_PixelSize", _pixelSize.y);
        
        BindColorAsTexture(2, 0, 0);

        _buffers[1]->RenderToFSQ();

        UnbindTexture(0);

        UnbindShader();
    }

    //Combine the original scene and the blurred buffers
    BindShader(4);

    buffer->BindColorAsTexture(0, 0, 0);
    BindColorAsTexture(1, 0, 1);

    _buffers[0]->RenderToFSQ();

    UnbindTexture(1);
    UnbindTexture(0);

    UnbindShader();
}

void BloomEffect::Reshape(unsigned width, unsigned height)
{
    _buffers[0]->Reshape(width, height);
    _buffers[1]->Reshape(unsigned(width / _downscale), unsigned(height / _downscale));
    _buffers[2]->Reshape(unsigned(width / _downscale), unsigned(height / _downscale));
    _buffers[3]->Reshape(width, height);
}
float BloomEffect::GetDownscale() const
{
    return _downscale;
}

float BloomEffect::GetThreshold() const
{
    return _threshold;
}

unsigned BloomEffect::GetPasses() const
{
    return _passes;
}

void BloomEffect::SetDownscale(float downscale)
{
    _downscale = downscale;
    Reshape(_buffers[0]->_width, _buffers[0]->_height);
}

void BloomEffect::SetThreshold(float threshold)
{
    _threshold = threshold;
}

void BloomEffect::SetPasses(unsigned passes)
{
    _passes = passes;
}