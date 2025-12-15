

#pragma once

#include "DataModel/Mouse.hpp"
#include "DataModel/Workspace.hpp"
namespace Aya
{

extern const char* const sPlayerMouse;

class PlayerMouse : public DescribedNonCreatable<PlayerMouse, Mouse, sPlayerMouse>
{
private:
    typedef DescribedNonCreatable<PlayerMouse, Mouse, sPlayerMouse> Super;

public:
    PlayerMouse();
    ~PlayerMouse();

    TextureId getIcon() const;
    void setIcon(const TextureId& value);
};

} // namespace Aya
