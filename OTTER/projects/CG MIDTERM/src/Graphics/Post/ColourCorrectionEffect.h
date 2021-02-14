#pragma once

#include "Graphics/Post/PostEffect.h"
#include "Graphics/LUT.h"

class ColourCorrectionEffect : public PostEffect
{
public:
	void Init(unsigned width, unsigned height) override;
	void ApplyEffect(PostEffect* buffer) override;
	void DrawToScreen() override;

	void SetLUT(LUT3D lut);
	LUT3D GetLUT();
private:
	LUT3D _LUT;
};