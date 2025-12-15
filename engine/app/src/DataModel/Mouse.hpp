

#pragma once

#include "Tree/Instance.hpp"
#include "DataModel/InputObject.hpp"
#include "Utility/TextureId.hpp"
#include "signal.hpp"

namespace Aya
{

class MouseCommand;
class PartInstance;
class PVInstance;
class Workspace;

extern const char* const sMouse;
class Mouse : public DescribedNonCreatable<Mouse, Instance, sMouse>
{
private:
public:
    Mouse();
    ~Mouse();

    Aya::signal<void()> moveSignal;
    Aya::signal<void()> idleSignal;
    Aya::signal<void()> button1DownSignal;
    Aya::signal<void()> button2DownSignal;
    Aya::signal<void()> button1UpSignal;
    Aya::signal<void()> button2UpSignal;
    Aya::signal<void()> wheelForwardSignal;
    Aya::signal<void()> wheelBackwardSignal;
    Aya::signal<void(std::string)> keyDownSignal;
    Aya::signal<void(std::string)> keyUpSignal;

    virtual void update(const shared_ptr<InputObject>& inputObject);

    // Returns the hit location, with the frame rotated so that the mouse shoots down -Z
    virtual CoordinateFrame getHit() const;

    // Returns the mouse origin, with the frame rotated so that the mouse shoots down -Z
    virtual CoordinateFrame getOrigin() const;

    virtual PartInstance* getTarget() const;

    shared_ptr<Instance> getTargetFilter() const;
    // TODO: Gotta rewrite RefPropDescriptor to take a shared_ptr
    Instance* getTargetFilterUnsafe() const
    {
        return targetFilter.lock().get();
    }
    virtual void setTargetFilter(Instance* value);

    virtual NormalId getTargetSurface() const;

    virtual Aya::RbxRay getUnitRay() const;

    void setCommand(MouseCommand* value);
    void setWorkspace(Workspace* workspace);

    virtual TextureId getIcon() const;
    virtual void setIcon(const TextureId& value);

    virtual int getX() const;
    virtual int getY() const;
    virtual int getViewSizeX() const;
    virtual int getViewSizeY() const;


    void cacheInputObject(const shared_ptr<InputObject>& inputObject);

protected:
    Workspace* workspace;
    MouseCommand* command;
    TextureId icon;
    shared_ptr<InputObject> lastEvent;
    weak_ptr<Instance> targetFilter;
    void setTargetFilterUnsafe(Instance* value); // solely called by child classes to fire changed event
    virtual void checkActive() const;
    Workspace* getWorkspace() const;
};


} // namespace Aya