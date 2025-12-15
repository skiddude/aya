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

class ParticleEmitter : public RenderNode
{
    typedef RenderNode Super;

public:
    ParticleEmitter(VisualEngine* visualEngine);
    ~ParticleEmitter();

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

    shared_ptr<Emitter> emitter;
    shared_ptr<Emitter> secondary;
    shared_ptr<Emitter> tertiary;
    CoordinateFrame emitterOffset;
    CoordinateFrame secondaryOffset;
    CoordinateFrame tertiaryOffset;
};

} // namespace Graphics
} // namespace Aya
