#pragma once

#include "Graphics/Post/PostEffect.h"

class FilmGrainEffect : public PostEffect
{
public:
	//Initializes framebuffer
	void Init(unsigned width, unsigned height) override;

	//Applies effect to this buffer
	void ApplyEffect(PostEffect* buffer) override;
	void IncrementAmount();

private:
	float _amount;
};