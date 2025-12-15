#pragma once

#include "Base/GfxPart.hpp"
#include "Utility/Extents.hpp"

#include "RenderQueue.hpp"

namespace Aya
{
class Contact;
}

namespace Aya
{
namespace Graphics
{

class RenderCamera;
class RenderQueue;
class VisualEngine;

class SpatialHashNode;

class CullableSceneNode : public Aya::GfxPart
{
public:
    static const bool hasGetFirstContact = false; // simulate __if_exists
    static Aya::Contact* getContact(CullableSceneNode* p0, CullableSceneNode* p1)
    {
        AYAASSERT(0);
        return NULL;
    } // shouldn't be called if hasGetFirstContact is false
    static Aya::Contact* getFirstContact()
    {
        AYAASSERT(NULL);
        return NULL;
    }
    static Aya::Contact* getNextContact(Aya::Contact* prev)
    {
        AYAASSERT(NULL);
        return NULL;
    }
    static int getNumContacts()
    {
        AYAASSERT(NULL);
        return 0;
    }
    static CullableSceneNode* getContactOther(int id)
    {
        AYAASSERT(NULL);
        return NULL;
    }

    bool requestFixed() const
    {
        return false;
    }
    const Extents& getFastFuzzyExtents()
    {
        return worldBounds;
    }

public:
    enum CullMode
    {
        CullMode_None,
        CullMode_Simple,
        CullMode_SpatialHash,
    };

    enum Flags
    {
        Flags_LightObject = 1 << 0,
        Flags_ShadowCaster = 1 << 1,
    };

    CullableSceneNode(VisualEngine* visualEngine, CullMode cullMode, unsigned int flags);
    ~CullableSceneNode();

    void setBlockCount(int blockCount)
    {
        this->blockCount = blockCount;
    }

    virtual void updateRenderQueue(RenderQueue& queue, const RenderCamera& camera, RenderQueue::Pass pass);

    float getSqDistanceToFocus() const
    {
        return sqDistanceToFocus;
    }

    const Extents& getWorldBounds() const
    {
        return worldBounds;
    }

    void updateWorldBounds(const Extents& aabb);

    VisualEngine* getVisualEngine() const
    {
        return visualEngine;
    }

    unsigned int getFlags() const
    {
        return flags;
    }

    bool updateIsCulledByFRM();

    Vector3 getCenter() const override
    {
        return worldBounds.center();
    }

private:
    VisualEngine* visualEngine;

    CullMode cullMode;
    unsigned int flags;

    int blockCount;
    float sqDistanceToFocus;

    Extents worldBounds;
};

} // namespace Graphics
} // namespace Aya
