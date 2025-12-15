


#include "DataModel/GameBasicSettings.hpp"
#include "DataModel/ForceField.hpp"
#include "DataModel/Workspace.hpp"
#include "DataModel/PartInstance.hpp"
#include "Base/Adorn.hpp"
#include "Draw.hpp"

FASTFLAG(RenderNewParticles2Enable);

namespace Aya
{

const char* const sForceField = "ForceField";

Reflection::PropDescriptor<ForceField, bool> ForceField::prop_Visible(
    "Visible", category_Appearance, &ForceField::getVisible, &ForceField::setVisible);

ForceField::ForceField()
    : visible(true)
    , cycle(0)
{
    setName("ForceField");

    if (Aya::GameBasicSettings::singleton().getVirtualVersion() >= Aya::GameBasicSettings::VERSION_2014)
    {
        invertCycle = cycles() / 2;
    }
    else
    {
        startTime = Time::now<Time::Fast>();
    }
}

void ForceField::setVisible(bool value)
{
    if (visible != value)
    {
        visible = value;
        raisePropertyChanged(prop_Visible);

        shouldRenderSetDirty();
    }
}

bool ForceField::askSetParent(const Instance* instance) const
{
    return (!Instance::fastDynamicCast<Workspace>(instance));
}

bool containsForceField(Instance* instance)
{
    for (size_t i = 0; i < instance->numChildren(); ++i)
    {
        Instance* child = instance->getChild(i);
        if (Instance::fastDynamicCast<ForceField>(child))
        {
            return true;
        }
    }
    return false;
}

bool ancestorContainsForceField(Instance* instance)
{
    AYAASSERT(instance);
    if (containsForceField(instance))
    {
        return true;
    }
    else
    {
        if (Instance* parent = instance->getParent())
        {
            if (!Instance::fastDynamicCast<Workspace>(parent))
            { // bail out at workspace level to prevent going through all workspace children
                return ancestorContainsForceField(parent);
            }
        }
        return false;
    }
}

bool ForceField::partInForceField(PartInstance* part)
{
    return ancestorContainsForceField(part);
}

void renderForceField(shared_ptr<Aya::Instance> descendant, Adorn* adorn, int cycle, int invertCycle)
{
    PartInstance* part = Instance::fastDynamicCast<PartInstance>(descendant.get());
    if (part && part->getLocalTransparencyModifier() < 0.99f) // don't render it for bricks too close to camera
    {
        int max = ForceField::cycles();
        int half = max / 2;

        int value = cycle < half ? cycle : max - cycle;
        int invertVal = invertCycle < half ? invertCycle : max - invertCycle;

        float percent = static_cast<float>(value) / static_cast<float>(half);
        float invertPercent = static_cast<float>(invertVal) / static_cast<float>(max);

        Vector3 pos = part->calcRenderingCoordinateFrame().translation;
        adorn->setObjectToWorldMatrix(pos);
        adorn->sphere(
            Sphere(Vector3::zero(), part->getPartSizeXml().magnitude()), Aya::Color4(0, 51.0f / 255.0f, 204.0f / 255.0f, (percent * 0.6f)) * .8f);
        adorn->sphere(Sphere(Vector3::zero(), part->getPartSizeXml().magnitude() * largeSize),
            Aya::Color4(0, invertPercent, 1.0f - invertPercent, ((invertPercent) * 0.45f)) * .8f);
    }
}


void ForceField::render3dAdorn(Adorn* adorn)
{

    Instance* parent = this->getParent();
    if (!torso.get())
        torso = shared_from(Instance::fastDynamicCast<PartInstance>(this->getParent()->findFirstChildByName2("Torso", true).get()));

    AYAASSERT(!Instance::fastDynamicCast<Workspace>(parent));

    if (!parent->fastDynamicCast<Workspace>() && torso.get()) // Second Hack
    {
        if (Aya::GameBasicSettings::singleton().getVirtualVersion() >= Aya::GameBasicSettings::VERSION_2014)
        {
            cycle = (cycle + 1) % cycles();
            invertCycle = (invertCycle + 1) % cycles();
        }
        else
        {
            const Time now = Time::now<Time::Fast>();
            double offset = (now - startTime).seconds() * 60.0;
            offset -= ::floor(offset);

            int cycle = (int)(offset * cycles());
            int invertCycle = cycle + cycles() / 2;
            if (invertCycle > cycles())
            {
                invertCycle -= cycles();
            }
        }

        // renderforcefield is used for the old forcefield rendering
        // -- new forcefields are handled by ParticleEmitter
        if (Aya::GameBasicSettings::singleton().getVirtualVersion() >= Aya::GameBasicSettings::VERSION_2014)
        {
            renderForceField(torso, adorn, cycle, invertCycle);
        }
    }
}


} // namespace Aya
