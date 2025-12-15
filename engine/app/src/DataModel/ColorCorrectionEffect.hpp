#pragma once

#include "PostEffect.hpp"

namespace Aya
{

extern const char* const sColorCorrectionEffect;

class ColorCorrectionEffect : public DescribedCreatable<ColorCorrectionEffect, PostEffect, sColorCorrectionEffect>
{
public:
    ColorCorrectionEffect();
    ~ColorCorrectionEffect();

    void setBrightness(float brightness);
    void setContrast(float contrast);
    void setSaturation(float saturation);
    void setTintColor(Color3 tint);

    float getBrightness() const { return m_brightness; }
    float getContrast() const { return m_contrast; }
    float getSaturation() const { return m_saturation; }
    Color3 getTintColor() const { return m_tintColor; }
private:
    float m_brightness = 0.0f;
    float m_contrast = 0.0f;
    float m_saturation = 0.0f;
    Color3 m_tintColor = Color3::white();
};

} // namespace Aya