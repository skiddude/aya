#pragma once

#include "Tree/Instance.hpp"

namespace Aya
{

extern const char* const sPostEffect;

class PostEffect : public DescribedNonCreatable<PostEffect, Instance, sPostEffect>
{
public:
    PostEffect();
    ~PostEffect();

    void setEnabled(bool enabled);
    bool isEnabled()  const { return m_enabled; }
private:
    bool m_enabled = true;
};

} // namespace Aya