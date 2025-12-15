#include "ColorCorrectionEffect.hpp"

namespace Aya
{

const char* const sColorCorrectionEffect = "ColorCorrectionEffect";

static Reflection::PropDescriptor<ColorCorrectionEffect, float> prop_brightness("Brightness", "State", &ColorCorrectionEffect::getBrightness, &ColorCorrectionEffect::setBrightness, Reflection::PropertyDescriptor::STANDARD);
static Reflection::PropDescriptor<ColorCorrectionEffect, float> prop_contrast("Contrast", "State", &ColorCorrectionEffect::getContrast, &ColorCorrectionEffect::setContrast, Reflection::PropertyDescriptor::STANDARD);
static Reflection::PropDescriptor<ColorCorrectionEffect, float> prop_saturation("Saturation", "State", &ColorCorrectionEffect::getSaturation, &ColorCorrectionEffect::setSaturation, Reflection::PropertyDescriptor::STANDARD);
static Reflection::PropDescriptor<ColorCorrectionEffect, Color3> prop_tintColor("TintColor", "State", &ColorCorrectionEffect::getTintColor, &ColorCorrectionEffect::setTintColor, Reflection::PropertyDescriptor::STANDARD);

ColorCorrectionEffect::ColorCorrectionEffect() 
    : m_brightness(0)
    , m_contrast(0)
    , m_saturation(0)
    , m_tintColor(Color3::white())
{
    setName(sColorCorrectionEffect);
}

ColorCorrectionEffect::~ColorCorrectionEffect()
{
    //
}

void ColorCorrectionEffect::setBrightness(float brightness)
{
    if (m_brightness == brightness)
        return;

    m_brightness = brightness;
    raisePropertyChanged(prop_brightness);
}

void ColorCorrectionEffect::setContrast(float contrast)
{
    if (m_contrast == contrast)
        return;

    m_contrast = contrast;
    raisePropertyChanged(prop_contrast);
}

void ColorCorrectionEffect::setSaturation(float saturation)
{
    if (m_saturation == saturation)
        return;

    m_saturation = -saturation; // NOTE: negative is intnetional, scenemanager treats this as grayscalelevel, too lazy to fix :P 
    raisePropertyChanged(prop_saturation);
}

void ColorCorrectionEffect::setTintColor(Color3 tint)
{
    if (m_tintColor == tint)
        return;

    m_tintColor = tint;
    raisePropertyChanged(prop_tintColor);
}

} // namespace Aya