#pragma once

#include "PostEffect.hpp"

namespace Aya
{

extern const char* const sBloomEffect;

class BloomEffect : public DescribedCreatable<BloomEffect, PostEffect, sBloomEffect>
{
public:
    BloomEffect();
    ~BloomEffect();

    void setIntensity(float intensity);
    void setSize(float size);
    void setThreshold(float threshold);

    float getIntensity() const { return m_intensity; }
    float getSize() const { return m_size; }
    float getThreshold() const { return m_threshold; }
private:
    float m_intensity;
    float m_size;
    float m_threshold;
};

} // namespace Aya