#pragma once

#include "Graphics/Post/PostEffect.h"

class ColourCorrectionEffect : public PostEffect
{
public:
	void Init(unsigned width, unsigned height) override;
	void ApplyEffect(PostEffect* buffer) override;
	void DrawToScreen() override;
	float GetIntensity() const;
	void SetIntensity(float intensity);
private:
	float _intensity = 1.0f;

};