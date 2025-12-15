#include "BloomEffect.hpp"

namespace Aya
{

const char* const sBloomEffect = "BloomEffect";

static Reflection::PropDescriptor<BloomEffect, float> prop_brightness("Intensity", "State", &BloomEffect::getIntensity, &BloomEffect::setIntensity, Reflection::PropertyDescriptor::STANDARD);
static Reflection::PropDescriptor<BloomEffect, float> prop_contrast("Size", "State", &BloomEffect::getSize, &BloomEffect::setSize, Reflection::PropertyDescriptor::STANDARD);
static Reflection::PropDescriptor<BloomEffect, float> prop_saturation("Threshold", "State", &BloomEffect::getThreshold, &BloomEffect::setThreshold, Reflection::PropertyDescriptor::STANDARD);


BloomEffect::BloomEffect() 
    : m_intensity(1)
    , m_size(24)
    , m_threshold(2)
{
    setName(sBloomEffect);
}

BloomEffect::~BloomEffect()
{
    //
}

void BloomEffect::setIntensity(float intensity)
{
    if (m_intensity == intensity)
        return;

    m_intensity = intensity;
    raisePropertyChanged(prop_brightness);
}

void BloomEffect::setSize(float size)
{
    if (m_size == size)
        return;

    m_size = size;
    raisePropertyChanged(prop_contrast);
}

void BloomEffect::setThreshold(float threshold)
{
    if (m_threshold == threshold)
        return;

    m_threshold = threshold;
    raisePropertyChanged(prop_saturation);
}

} // namespace Aya