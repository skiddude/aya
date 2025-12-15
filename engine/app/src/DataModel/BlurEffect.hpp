#pragma once

#include "PostEffect.hpp"

namespace Aya
{

extern const char* const sBlurEffect;

class BlurEffect : public DescribedCreatable<BlurEffect, PostEffect, sBlurEffect>
{
public:
    BlurEffect();
    ~BlurEffect();

    void setSize(float size);
    float getSize() const { return m_size; }
private:
    float m_size = 4.0f;
};

} // namespace Aya