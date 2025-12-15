#include "BlurEffect.hpp"

namespace Aya
{

const char* const sBlurEffect = "BlurEffect";

static Reflection::PropDescriptor<BlurEffect, float> prop_size("Size", "State", &BlurEffect::getSize, &BlurEffect::setSize, Reflection::PropertyDescriptor::STANDARD);

BlurEffect::BlurEffect()
    : m_size(4.0f)
{
    setName(sBlurEffect);
}

BlurEffect::~BlurEffect()
{
    //
}

void BlurEffect::setSize(float size)
{
    if (m_size == size)
        return;

    m_size = size;
    raisePropertyChanged(prop_size);
}

} // namespace Aya