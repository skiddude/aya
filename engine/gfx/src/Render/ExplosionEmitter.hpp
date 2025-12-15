#pragma once

#include "Utility/G3DCore.hpp"

#include "RenderNode.hpp"

namespace Aya
{
class PartInstance;
class Instance;
} // namespace Aya

namespace Aya
{
namespace Graphics
{

class Emitter;

class ExplosionEmitter : public RenderNode
{
    typedef RenderNode Super;

public:
    ExplosionEmitter(VisualEngine* visualEngine);
    ~ExplosionEmitter();

    void bind(const shared_ptr<Aya::PartInstance>& part, const shared_ptr<Aya::Instance>& instance);

    // GfxBinding overrides
    virtual void invalidateEntity();
    virtual void updateEntity(bool assetsUpdated);
    virtual void unbind();

    // GfxPart overrides
    virtual void updateCoordinateFrame(bool recalcLocalBounds);

    virtual void updateRenderQueue(RenderQueue& queue, const RenderCamera& camera, RenderQueue::Pass pass);

private:
    void onCombinedSignalEx(Instance::CombinedSignalType type, const Instance::ICombinedSignalData* data);
    void onPropertyChangedEx(const Aya::Reflection::PropertyDescriptor* descriptor);
    void onAncestorChangedEx();
    void onSleepingChangedEx(bool sleeping);

    void applySettings();

    shared_ptr<Aya::PartInstance> part;
    shared_ptr<Aya::Instance> effect;
    CoordinateFrame transform;
    Extents localBox;
    bool dirty;
    bool enabled;

    enum
    {
        kCore,
        kChunks,
        kShockwave,
        kSmoke,
        kImplosion,

        Num_Emitters,
    };
    scoped_ptr<Emitter> emitters[Num_Emitters];
};

} // namespace Graphics
} // namespace Aya
