

#pragma once
#include "Tree/Instance.hpp"
#include "Tree/Verb.hpp"

namespace Aya
{
class Workspace;
class Mouse;

extern const char* const sStudioTool;

class StudioTool : public DescribedNonCreatable<StudioTool, Instance, sStudioTool>
{
protected:
    shared_ptr<Mouse> onEquipping(Workspace* workspace);
    bool enabled;

public:
    StudioTool();

    bool getEnabled() const
    {
        return enabled;
    }
    void setEnabled(bool);

    void activate();
    void deactivate();

    void equip(Workspace*);
    void unequip();

    Aya::signal<void(shared_ptr<Instance>)> equippedSignal;
    Aya::signal<void()> activatedSignal;
    Aya::signal<void()> unequippedSignal;
    Aya::signal<void()> deactivatedSignal;
};
} // namespace Aya
