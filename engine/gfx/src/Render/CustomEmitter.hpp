#pragma once

#include "Utility/G3DCore.hpp"

#include "RenderNode.hpp"

namespace Aya
{
class PartInstance;
class Instance;
class CustomParticleEmitter;
} // namespace Aya

namespace Aya
{
namespace Graphics
{

class Emitter;

class CustomEmitter : public RenderNode
{
    typedef RenderNode Super;

public:
    CustomEmitter(VisualEngine* visualEngine);
    ~CustomEmitter();

    void bind(const shared_ptr<Aya::PartInstance>& part, const shared_ptr<Aya::Instance>& instance);

    // GfxBinding overrides
    virtual void invalidateEntity();
    virtual void updateEntity(bool assetsUpdated);
    virtual void unbind();

    // GfxPart overrides
    virtual void updateCoordinateFrame(bool recalcLocalBounds);

    virtual void updateRenderQueue(RenderQueue& queue, const RenderCamera& camera, RenderQueue::Pass pass);

private:
    enum
    {
        kDirty_None = 0,
        kDirty_Invalidate = 1,
        kDirty_Transform = 2,
        kDirty_Fast = 4,
        kDirty_Slow = 8,
        kDirty_Curves = 16,
        kDirty_All = ~(unsigned)kDirty_Invalidate,
    }; // dirty flags

    typedef const Aya::Reflection::PropertyDescriptor* pd;

    void onCombinedSignalEx(Instance::CombinedSignalType type, const Instance::ICombinedSignalData* data);
    void onPropertyChangedEx(const Aya::Reflection::PropertyDescriptor* descriptor);
    void onAncestorChangedEx();
    void onSleepingChangedEx(bool sleeping);
    void requestEmit(int particleCount);

    void onParentSize(pd p);

    void applyAllSettings();

    shared_ptr<Aya::PartInstance> part;
    shared_ptr<Aya::CustomParticleEmitter> effect;
    CoordinateFrame transform;
    Extents localBox;
    bool enabled;

    unsigned dirtyFlags;

    scoped_ptr<Emitter> emitter;

    int requestedEmitCount;
};

} // namespace Graphics
} // namespace Aya
