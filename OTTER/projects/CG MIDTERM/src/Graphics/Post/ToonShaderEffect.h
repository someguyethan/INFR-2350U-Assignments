#pragma once

#include "Graphics/Post/PostEffect.h"

class ToonShaderEffect : public PostEffect
{
public:
	void Init(unsigned width, unsigned height) override;
	void ApplyEffect(PostEffect* buffer) override;
	void DrawToScreen() override;
	int  GetBands();
	void SetBands(int bands);
private:
	int _bands = 10;
};