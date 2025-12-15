#include "PostEffect.hpp"

namespace Aya
{

const char* const sPostEffect = "PostEffect";

static Reflection::PropDescriptor<PostEffect, bool> prop_enabled("Enabled", "State", &PostEffect::isEnabled, &PostEffect::setEnabled, Reflection::PropertyDescriptor::STANDARD);

PostEffect::PostEffect() : m_enabled(true)
{
    //
}

PostEffect::~PostEffect()
{
    //
}

void PostEffect::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;

    m_enabled = enabled;
    raisePropertyChanged(prop_enabled);
}

} // namespace Aya