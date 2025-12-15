
#include "Render/ParticleEmitter.hpp"

#include "DataModel/Smoke.hpp"
#include "DataModel/Sparkles.hpp"
#include "DataModel/Fire.hpp"
#include "DataModel/ForceField.hpp"
#include "DataModel/Explosion.hpp"

#include "Render/SceneUpdater.hpp"
#include "Render/Util.hpp"

#include "Render/Emitter.hpp"
#include "Render/TextureManager.hpp"

#include "Render/VisualEngine.hpp"

#include "DataModel/GameBasicSettings.hpp"

DYNAMIC_FASTFLAGVARIABLE(SphericalSparklesEmission, false)

namespace Aya
{
namespace Graphics
{

ParticleEmitter::ParticleEmitter(VisualEngine* visualEngine)
    : Super(visualEngine, CullMode_SpatialHash)
    , dirty(false)
    , enabled(false)
    , localBox(Vector3::zero(), Vector3::zero())
{
}

ParticleEmitter::~ParticleEmitter()
{
    unbind();

    // notify scene updater about destruction so that the pointer to ParticleEmitter is no longer stored
    getVisualEngine()->getSceneUpdater()->notifyDestroyed(this);
}

void ParticleEmitter::onSleepingChangedEx(bool sleeping)
{
    if (sleeping)
    {
        getVisualEngine()->getSceneUpdater()->notifySleeping(this);
    }
    else
    {
        getVisualEngine()->getSceneUpdater()->notifyAwake(this);
    }
}

void ParticleEmitter::updateCoordinateFrame(bool recalcLocalBounds)
{
    CoordinateFrame frame = part ? part->calcRenderingCoordinateFrame() : CoordinateFrame();

    if (Aya::Explosion* expl = Instance::fastDynamicCast<Aya::Explosion>(effect.get()))
    {
        frame.rotation = Matrix3::identity();
        frame.translation = expl->getPosition();
    }

    if (!dirty && transform.fuzzyEq(frame))
    {
        // Nothing to update
        return;
    }

    transform = frame;

    // Update particle transforms
    if (emitter)
    {
        emitter->transform() = transform * emitterOffset;
    }

    if (secondary)
    {
        secondary->transform() = transform * secondaryOffset;
    }

    if (tertiary)
    {
        tertiary->transform() = transform * tertiaryOffset;
    }

    // Update world-space extents
    updateWorldBounds(localBox.toWorldSpace(transform));
}

void ParticleEmitter::onCombinedSignalEx(Instance::CombinedSignalType type, const Instance::ICombinedSignalData* data)
{
    switch (type)
    {
    case Instance::PROPERTY_CHANGED:
        onPropertyChangedEx(boost::polymorphic_downcast<const Instance::PropertyChangedSignalData*>(data)->propertyDescriptor);
        break;
    case Instance::ANCESTRY_CHANGED:
        onAncestorChangedEx();
        break;
    default:
        break;
    }
}

void ParticleEmitter::onPropertyChangedEx(const Aya::Reflection::PropertyDescriptor* descriptor)
{
    invalidateEntity();
}

void ParticleEmitter::onAncestorChangedEx()
{
    shared_ptr<Instance> effectCopy = effect;

    // Remove me from the scene if I am being removed from the Workspace
    if (!isInWorkspace(effectCopy.get()))
    {
        // will cause a delete on next updateEntity()
        zombify();
    }
    else
    {
        unbind();

        Aya::PartInstance* parent = Aya::Instance::fastDynamicCast<Aya::PartInstance>(effectCopy->getParent());
        shared_ptr<Aya::PartInstance> part = shared_from(parent);

        bind(part, effectCopy);
    }
}

void ParticleEmitter::bind(const shared_ptr<Aya::PartInstance>& part, const shared_ptr<Aya::Instance>& instance)
{
    AYAASSERT(!this->part && !this->effect);
    AYAASSERT(instance);

    this->part = part;
    this->effect = instance;

    connections.push_back(instance->combinedSignal.connect(boost::bind(&ParticleEmitter::onCombinedSignalEx, this, _1, _2)));

    if (part)
    {
        connections.push_back(part->onDemandWrite()->sleepingChangedSignal.connect(boost::bind(&ParticleEmitter::onSleepingChangedEx, this, _1)));

        // we just connected, so sync up the state.
        onSleepingChangedEx(part->getSleeping());
    }

    invalidateEntity();
}

void ParticleEmitter::unbind()
{
    Super::unbind();

    part.reset();
    effect.reset();
}

void ParticleEmitter::invalidateEntity()
{
    if (!dirty)
    {
        dirty = true;

        getVisualEngine()->getSceneUpdater()->queueInvalidatePart(this);
    }
}

void ParticleEmitter::updateEntity(bool assetsUpdated)
{
    if (connections.empty()) // zombified.
    {
        getVisualEngine()->getSceneUpdater()->destroyAttachment(this);
        return;
    }

    applySettings();
    updateCoordinateFrame(true);

    dirty = false;
}

void ParticleEmitter::updateRenderQueue(RenderQueue& queue, const RenderCamera& camera, RenderQueue::Pass pass)
{

    // VirtualVersion swapping
    if (effect)
    {
        if (effect->isA<Aya::ForceField>() && enabled) // add more || effect->isA<particle> for future effects
        {
            bool isNewForcefield = !emitter->isLegacyEmitter();
            bool showNewForcefield = Aya::GameBasicSettings::singleton().getVirtualVersion() < Aya::GameBasicSettings::VERSION_2014;

            // Aya::StandardOut::singleton()->printf(Aya::MESSAGE_INFO, "isNewForcefield: %d, showNewForcefield: %d", isNewForcefield,
            // showNewForcefield);

            if (!emitter)
                emitter.reset(new Emitter(getVisualEngine()));

            if (!secondary)
                secondary.reset(new Emitter(getVisualEngine()));

            if (!tertiary)
                tertiary.reset(new Emitter(getVisualEngine()));

            if (!isNewForcefield && showNewForcefield)
            {
                static const Vector3 one(1, 1, 1);

                // Switch to new forcefield
                if (emitter)
                {
                    Emitter::Appearance a = {};
                    a.alphaStripTexture = "ayaasset://textures/particles/common_alpha.jxl";
                    a.colorStripTexture = "ayaasset://textures/particles/forcefield_vortex_color.jxl";
                    a.mainTexture = "ayaasset://textures/particles/forcefield_vortex_main.jxl";
                    a.colorStripBaseline = -1;
                    a.shader = Emitter::Shader_Add;
                    a.blendCode = Emitter::Blend_AlphaOne;

                    emitter->setAppearance(a);
                    emitter->setDampening(0);
                    emitter->setGlobalForce(Vector3(0, -2, 0));
                    emitter->setGrowth(1.6f * Vector2(1, 1));
                    emitter->setLocalForce(Vector3(0, 5, 0));
                    emitter->setRotation(Vector2(400 / 57.3f, 400 / 57.3f));
                    emitter->setSizeX(Vector2(2, 2));
                    emitter->setSizeY(Vector2(0.2f, 0.2f));

                    emitter->setSpeed(2.0f * Vector2(1, 1));
                    emitter->setSpin(Vector2(200 / 57.3f, 200 / 57.3f));
                    emitter->setSpread(Vector2(10 / 57.3f, 10 / 57.3f));

                    emitter->setEmissionRate(40 * enabled);
                    emitter->setEmitterShape(0, Box(-0.5f * one, 0.5f * one));
                    emitter->setLife(Vector2(1, 1));
                    emitter->setModulateColor(Vector4(1, 1, 1, 1));
                    emitterOffset = Vector3(0, -1.9f, 0);
                    emitter->setInheritMotion(1);
                    emitter->setLockedToLocalSpace(true);
                    emitter->setLegacyEmitter(false);
                }

                if (secondary)
                {
                    Emitter::Appearance a = {};
                    a.alphaStripTexture = "ayaasset://textures/particles/common_alpha.jxl";
                    a.colorStripTexture = "ayaasset://textures/particles/forcefield_vortex_color.jxl";
                    a.mainTexture = "ayaasset://textures/sparkle.png";
                    a.colorStripBaseline = -1;
                    a.shader = Emitter::Shader_Add;
                    a.blendCode = Emitter::Blend_AlphaOne;

                    secondary->setAppearance(a);
                    secondary->setDampening(0);
                    secondary->setGlobalForce(Vector3(0, 0, 0));
                    secondary->setGrowth(0.14f * Vector2(1, 1));
                    secondary->setLocalForce(Vector3(0, 2, 0));
                    secondary->setRotation(Vector2(0, 0));
                    secondary->setSizeX(0.04f * Vector2(1, 1));
                    secondary->setSizeY(0.04f * Vector2(1, 1));

                    secondary->setSpeed(1.5f * Vector2(1, 1));
                    secondary->setSpin(Vector2(-500 / 57.3f, 500 / 57.3f));
                    secondary->setSpread(Vector2(30 / 57.3f, 30 / 57.3f));

                    secondary->setEmissionRate(12 * enabled);
                    secondary->setEmitterShape(0, Box(-0.8f * one, 0.8f * one));
                    secondary->setLife(Vector2(1.3f, 1.3f));
                    secondary->setModulateColor(Vector4(1, 1, 1, 1));
                    secondary->setLockedToLocalSpace(true);
                    secondaryOffset = Vector3(0, 0, 0);
                }

                if (tertiary) // glow
                {
                    Emitter::Appearance a = {};
                    a.alphaStripTexture = "ayaasset://textures/particles/forcefield_glow_alpha.jxl";
                    a.colorStripTexture = "ayaasset://textures/particles/forcefield_glow_color.jxl";
                    a.mainTexture = "ayaasset://textures/particles/forcefield_glow_main.jxl";
                    a.colorStripBaseline = -1;
                    a.shader = Emitter::Shader_Crazy;
                    a.blendCode = Emitter::Blend_PremultipliedAlpha;

                    tertiary->setAppearance(a);
                    tertiary->setDampening(0);
                    tertiary->setGlobalForce(Vector3(0, 0, 0));
                    tertiary->setGrowth(0.5f * Vector2(1, 1));
                    tertiary->setLocalForce(Vector3(0, 0, 0));
                    tertiary->setRotation(Vector2(0, 0));
                    tertiary->setSizeX(4.4f * Vector2(1, 1));
                    tertiary->setSizeY(4.4f * Vector2(1, 1));

                    tertiary->setSpeed(0.0f * Vector2(1, 1));
                    tertiary->setSpin(Vector2(200 / 57.3f, 200 / 57.3f));
                    tertiary->setSpread(Vector2(2 / 57.3f, 2 / 57.3f));

                    tertiary->setEmissionRate(15 * enabled);
                    tertiary->setEmitterShape(0, Box(Vector3(0, 0, 0), Vector3(0, 0, 0)));
                    tertiary->setLife(Vector2(2, 2));
                    tertiary->setModulateColor(Vector4(1, 1, 1, 1));
                    tertiaryOffset = Vector3(0, -0.5f, 0);
                    tertiary->setZOffset(2.5f);
                    tertiary->setInheritMotion(1);
                    tertiary->setBrightenOnThrottle(1);
                    tertiary->setBlendRatio(0.15f);
                    tertiary->setLockedToLocalSpace(true);
                }
            }
            else if (isNewForcefield && !showNewForcefield)
            {
                static const Vector3 one(1, 1, 1);

                if (emitter) // vortex
                {
                    Emitter::Appearance a = {};
                    a.alphaStripTexture = "ayaasset://textures/particles/common_alpha.jxl";
                    a.mainTexture = "ayaasset://textures/glow.png";
                    a.shader = Emitter::Shader_Crazy;
                    a.blendCode = Emitter::Blend_AlphaOne;

                    emitter->setAppearance(a);
                    emitter->setDampening(0);
                    emitter->setGlobalForce(Vector3(0, 0, 0));
                    emitter->setGrowth(0.5f * Vector2(1, 1));
                    emitter->setLocalForce(Vector3(0, 0, 0));
                    emitter->setRotation(Vector2(0, 0));
                    emitter->setSizeX(5.5f * Vector2(1, 1));
                    emitter->setSizeY(5.5f * Vector2(1, 1));

                    emitter->setSpeed(0.0f * Vector2(1, 1));
                    emitter->setSpin(Vector2(100 / 57.3f, 100 / 57.3f));
                    emitter->setSpread(Vector2(2 / 57.3f, 2 / 57.3f));

                    emitter->setEmissionRate(1 * enabled);
                    emitter->setEmitterShape(0, Box(Vector3(0, 0, 0), Vector3(0, 0, 0)));
                    emitter->setLife(Vector2(2, 2));
                    emitter->setModulateColor(Vector4(1, 1, 1, 1));
                    emitterOffset = Vector3(0, -0.2f, 0);
                    emitter->setZOffset(2.5f);
                    emitter->setInheritMotion(1);
                    emitter->setBrightenOnThrottle(1);
                    emitter->setBlendRatio(0.15f);
                    emitter->setLockedToLocalSpace(true);
                    emitter->setLegacyEmitter(true);
                }

                if (secondary) // stars
                {
                    Emitter::Appearance a = {};
                    a.alphaStripTexture = "ayaasset://textures/particles/common_alpha.jxl";
                    a.mainTexture = "ayaasset://textures/sparkleLight.png";
                    a.shader = Emitter::Shader_Add;
                    a.blendCode = Emitter::Blend_AlphaOne;

                    secondary->setAppearance(a);
                    secondary->setDampening(0);
                    secondary->setGlobalForce(Vector3(0, 0, 0));
                    secondary->setGrowth(0.05f * Vector2(1, 1));
                    secondary->setLocalForce(Vector3(0, 2, 0));
                    secondary->setRotation(Vector2(0, 0));
                    secondary->setSizeX(0.10f * Vector2(1, 1));
                    secondary->setSizeY(1.50f * Vector2(1, 1));

                    secondary->setSpeed(3.0f * Vector2(1, 1));
                    secondary->setSpin(Vector2(-500 / 57.3f, 500 / 57.3f));
                    secondary->setSpread(Vector2(30 / 57.3f, 30 / 57.3f));

                    secondary->setEmissionRate(12 * enabled);
                    secondary->setEmitterShape(0, Box(-0.8f * one, 0.8f * one));
                    secondary->setLife(Vector2(1.3f, 1.3f));
                    secondary->setModulateColor(Vector4(1, 1, 1, 1));
                    secondary->setLockedToLocalSpace(true);
                    secondaryOffset = Vector3(0, -1, 0);
                }

                if (tertiary)
                {
                    Emitter::Appearance a = {};
                    a.alphaStripTexture = "ayaasset://textures/particles/common_alpha.jxl";
                    a.mainTexture = "ayaasset://textures/sparkleLight2.png";
                    a.shader = Emitter::Shader_Add;
                    a.blendCode = Emitter::Blend_AlphaOne;

                    tertiary->setAppearance(a);
                    tertiary->setDampening(0);
                    tertiary->setGlobalForce(Vector3(0, 0, 0));
                    tertiary->setGrowth(0.05f * Vector2(1, 1));
                    tertiary->setLocalForce(Vector3(0, 2, 0));
                    tertiary->setRotation(Vector2(0, 0));
                    tertiary->setSizeX(0.50f * Vector2(1, 1));
                    tertiary->setSizeY(1.50f * Vector2(1, 1));

                    tertiary->setSpeed(1.5f * Vector2(1, 1));
                    tertiary->setSpin(Vector2(-500 / 57.3f, 500 / 57.3f));
                    tertiary->setSpread(Vector2(30 / 57.3f, 30 / 57.3f));

                    tertiary->setEmissionRate(12 * enabled);
                    tertiary->setEmitterShape(0, Box(-0.8f * one, 0.8f * one));
                    tertiary->setLife(Vector2(0.9f, 0.9f));
                    tertiary->setModulateColor(Vector4(1, 1, 1, 1));
                    tertiary->setLockedToLocalSpace(true);
                    tertiaryOffset = Vector3(0, -1, 0);
                } // extra sparkles
            }
        }
    }

    if (!enabled && emitter && !emitter->pcount())
        emitter.reset();

    if (!enabled && secondary && !secondary->pcount())
        secondary.reset();

    if (!enabled && tertiary && !tertiary->pcount())
        secondary.reset();

    if (emitter)
        emitter->draw(queue);

    if (secondary)
        secondary->draw(queue);

    if (tertiary)
        tertiary->draw(queue);

    // Render bounding box
    if (getVisualEngine()->getSettings()->getDebugShowBoundingBoxes() && (emitter || secondary || tertiary))
        debugRenderBoundingBox();
}

void ParticleEmitter::applySettings()
{
    if (!effect)
        return;

    static const Vector4 zzz(0, 0, 0, 0);
    static const Vector3 one(1, 1, 1);
    emitterOffset = CoordinateFrame();
    secondaryOffset = CoordinateFrame();
    tertiaryOffset = CoordinateFrame();

    if (Aya::Sparkles* spk = effect->fastDynamicCast<Aya::Sparkles>())
    {
        if (Aya::GameBasicSettings::singleton().getVirtualVersion() < Aya::GameBasicSettings::VERSION_2015) // 2016
        {
            enabled = spk->getEnabled();
            Vector4 c = Color4(spk->getColor());
            c.w = 0.9f; // spk->getAlpha();

            if (enabled && !emitter)
                emitter.reset(new Emitter(getVisualEngine()));

            if (emitter)
            {
                Emitter::Appearance a = {};
                a.alphaStripTexture = "ayaasset://textures/particles/common_alpha.jxl";
                a.colorStripTexture = "ayaasset://textures/particles/sparkles_color.jxl";
                a.mainTexture = "ayaasset://textures/particles/sparkles_main.jxl";
                a.colorStripBaseline = -1;
                a.shader = Emitter::Shader_CrazySparkles;
                a.blendCode = Emitter::Blend_PremultipliedAlpha;

                emitter->setAppearance(a);
                emitter->setDampening(0.2f);
                emitter->setGlobalForce(Vector3(0, 0, 0));
                emitter->setGrowth(Vector2(0.5f, 0.5f));
                emitter->setLocalForce(Vector3(0, 1, 0));
                emitter->setRotation(Vector2(-90 / 57.3f, 90 / 57.3f));
                emitter->setSizeX(Vector2(0.37f, 0.37f));
                emitter->setSizeY(Vector2(0.37f, 0.37f));
                emitter->setSpeed(Vector2(5, 5));
                emitter->setSpin(Vector2(40 / 57.3f, 100 / 57.3f));

                if (!DFFlag::SphericalSparklesEmission)
                {
                    emitter->setSpread(Vector2(100 / 57.3f, 100 / 57.3f));
                }
                else
                {
                    emitter->setSpread(Math::pif() * Vector2(1, 1));
                }

                emitter->setEmissionRate(30 * enabled);
                emitter->setEmitterShape(0, Box(Vector3(-0.2f, -0.2f, -0.2f), Vector3(0.2f, 0.2f, 0.2f)));
                emitter->setLife(Vector2(1.3f, 1.3f));

                emitter->setModulateColor(c);
                emitter->setBlendRatio(0.6f);
            }

            if (enabled && !secondary)
                secondary.reset(new Emitter(getVisualEngine()));

            if (secondary)
            {
                Emitter::Appearance a = {};
                a.alphaStripTexture = "ayaasset://textures/particles/common_alpha.jxl";
                a.colorStripTexture = "ayaasset://textures/particles/sparkles_color.jxl";
                a.mainTexture = "ayaasset://textures/particles/sparkles_main.jxl";
                a.colorStripBaseline = -1;
                a.shader = Emitter::Shader_CrazySparkles;
                a.blendCode = Emitter::Blend_PremultipliedAlpha;

                secondary->setAppearance(a);
                secondary->setDampening(2);
                secondary->setGlobalForce(Vector3(0, 0, 0));
                secondary->setGrowth(Vector2(0.2f, 0.2f));
                secondary->setLocalForce(Vector3(0, 0, 0));
                secondary->setRotation(Vector2(-90 / 57.3f, 90 / 57.3f));
                secondary->setSizeX(Vector2(.1f, .1f));
                secondary->setSizeY(Vector2(.1f, .1f));
                secondary->setSpeed(Vector2(8, 8));
                secondary->setSpin(Vector2(-500 / 57.3f, 500 / 57.3f));

                if (!DFFlag::SphericalSparklesEmission)
                {
                    secondary->setSpread(Vector2(150 / 57.3f, 150 / 57.3f));
                    secondaryOffset = CoordinateFrame(Vector3(0, 4.f, 0));
                }
                else
                {
                    secondary->setSpread(Math::pif() * Vector2(1, 1));
                    secondaryOffset = CoordinateFrame(Vector3(0, 0, 0));
                }

                secondary->setEmissionRate(5 * enabled);
                secondary->setEmitterShape(0, Box(Vector3(-0.2f, -0.2f, -0.2f), Vector3(0.2f, 0.2f, 0.2f)));
                secondary->setLife(Vector2(1.7, 1.7));
                secondary->setModulateColor(c);
                secondary->setBlendRatio(0.6f);
            }

            tertiary.reset();
        }
        else
        {
            enabled = spk->getEnabled();
            Vector4 c = Color4(spk->getColor());
            c.w = 0.9f; // spk->getAlpha();

            if (enabled && !emitter)
                emitter.reset(new Emitter(getVisualEngine()));

            if (emitter)
            {
                Emitter::Appearance a = {};
                a.alphaStripTexture = "ayaasset://textures/particles/common_alpha.jxl";
                a.colorStripTexture = "ayaasset://textures/particles/sparkles_color.jxl";
                a.mainTexture = "ayaasset://textures/particles/sparkles_main.jxl";
                a.colorStripBaseline = -1;
                a.shader = Emitter::Shader_CrazySparkles;
                a.blendCode = Emitter::Blend_PremultipliedAlpha;

                emitter->setAppearance(a);
                emitter->setDampening(0.2f);
                emitter->setGlobalForce(Vector3(0, 0, 0));
                emitter->setGrowth(Vector2(0.5f, 0.5f));
                emitter->setLocalForce(Vector3(0, 1, 0));
                emitter->setRotation(Vector2(-90 / 57.3f, 90 / 57.3f));
                emitter->setSizeX(Vector2(0.37f, 0.37f));
                emitter->setSizeY(Vector2(0.37f, 0.37f));
                emitter->setSpeed(Vector2(5, 5));
                emitter->setSpin(Vector2(40 / 57.3f, 100 / 57.3f));

                if (!DFFlag::SphericalSparklesEmission)
                {
                    emitter->setSpread(Vector2(100 / 57.3f, 100 / 57.3f));
                }
                else
                {
                    emitter->setSpread(Math::pif() * Vector2(1, 1));
                }

                emitter->setEmissionRate(2 * enabled);
                emitter->setEmitterShape(0, Box(Vector3(-0.2f, -0.2f, -0.2f), Vector3(0.2f, 0.2f, 0.2f)));
                emitter->setLife(Vector2(1.3f, 1.3f));

                emitter->setModulateColor(c);
                emitter->setBlendRatio(0.6f);
            }

            if (enabled && !secondary)
                secondary.reset(new Emitter(getVisualEngine()));

            if (secondary)
            {
                Emitter::Appearance a = {};
                a.alphaStripTexture = "ayaasset://textures/particles/common_alpha.jxl";
                a.colorStripTexture = "ayaasset://textures/particles/sparkles_color.jxl";
                a.mainTexture = "ayaasset://textures/particles/sparkles_main.jxl";
                a.colorStripBaseline = -1;
                a.shader = Emitter::Shader_CrazySparkles;
                a.blendCode = Emitter::Blend_PremultipliedAlpha;

                secondary->setAppearance(a);
                secondary->setDampening(2);
                secondary->setGlobalForce(Vector3(0, 0, 0));
                secondary->setGrowth(Vector2(0.2f, 0.2f));
                secondary->setLocalForce(Vector3(0, 0, 0));
                secondary->setRotation(Vector2(-90 / 57.3f, 90 / 57.3f));
                secondary->setSizeX(Vector2(.1f, .1f));
                secondary->setSizeY(Vector2(.1f, .1f));
                secondary->setSpeed(Vector2(8, 8));
                secondary->setSpin(Vector2(-500 / 57.3f, 500 / 57.3f));

                if (!DFFlag::SphericalSparklesEmission)
                {
                    secondary->setSpread(Vector2(150 / 57.3f, 150 / 57.3f));
                    secondaryOffset = CoordinateFrame(Vector3(0, 4.f, 0));
                }
                else
                {
                    secondary->setSpread(Math::pif() * Vector2(1, 1));
                    secondaryOffset = CoordinateFrame(Vector3(0, 0, 0));
                }

                secondary->setEmissionRate(1 * enabled);
                secondary->setEmitterShape(0, Box(Vector3(-0.2f, -0.2f, -0.2f), Vector3(0.2f, 0.2f, 0.2f)));
                secondary->setLife(Vector2(1.7, 1.7));
                secondary->setModulateColor(c);
                secondary->setBlendRatio(0.6f);
            }

            tertiary.reset();
        }
    }
    else if (Aya::Smoke* smk = effect->fastDynamicCast<Aya::Smoke>())
    {
        if (Aya::GameBasicSettings::singleton().getVirtualVersion() < Aya::GameBasicSettings::VERSION_2015) // 2016
        {
            enabled = smk->getEnabled();

            if (enabled && !emitter)
                emitter.reset(new Emitter(getVisualEngine()));

            if (emitter)
            {
                float size = smk->getClampedSize();
                float maxSize = 10 + size;
                float rise = smk->getClampedRiseVelocity();
                float opacity = smk->getClampedOpacity();
                float life = 5;
                Vector4 c0(Color4(smk->getColor(), opacity));
                Vector4 c1(Color4(smk->getColor(), 0.0f));

                Emitter::Appearance a = {};
                a.alphaStripTexture = "ayaasset://textures/particles/common_alpha.jxl";
                a.colorStripTexture = "ayaasset://textures/particles/smoke_color.jxl";
                a.mainTexture = "ayaasset://textures/particles/smoke_main.jxl";
                a.colorStripBaseline = -1;
                a.shader = Emitter::Shader_Modulate;
                a.blendCode = Emitter::Blend_AlphaBlend;

                emitter->setAppearance(a);
                emitter->setDampening(0.1f);
                emitter->setGlobalForce(Vector3(0, 0, 0.4));
                emitter->setGrowth((maxSize - size) / life * Vector2(1, 1));
                emitter->setLocalForce(Vector3(0, 0, 0));
                emitter->setRotation(Vector2(-90 / 57.3f, 90 / 57.3f));
                emitter->setSizeX(size * Vector2(1, 1));
                emitter->setSizeY(size * Vector2(1, 1));
                emitter->setSpeed(rise * Vector2(0.9f, 1));
                emitter->setSpin(Vector2(-20 / 57.3f, 20 / 57.3f));
                emitter->setSpread(Vector2(30 / 57.3f, 30 / 57.3f));

                emitter->setEmissionRate(7 * enabled);
                emitter->setEmitterShape(0, Box(Vector3(-1, -1, -1), Vector3(1, 1, 1)));
                emitter->setLife(life * Vector2(1, 1));

                emitter->setModulateColor(c0);
            }

            tertiary.reset();
        }
        else
        {
            // 2013
            enabled = smk->getEnabled();

            if (enabled && !emitter)
                emitter.reset(new Emitter(getVisualEngine()));

            if (emitter)
            {
                float size = smk->getClampedSize();
                float maxSize = 4 + size;
                float rise = smk->getClampedRiseVelocity();
                float opacity = smk->getClampedOpacity();
                float life = 1;
                Vector4 c0(Color4(smk->getColor(), opacity));
                Vector4 c1(Color4(smk->getColor(), 0.0f));

                Emitter::Appearance a = {};
                a.alphaStripTexture = "ayaasset://textures/particles/common_alpha.jxl";
                a.colorStripTexture = "ayaasset://textures/particles/smoke_color.jxl";
                a.mainTexture = "ayaasset://textures/particles/smoke_main.jxl";
                a.colorStripBaseline = -1;
                a.shader = Emitter::Shader_Modulate;
                a.blendCode = Emitter::Blend_AlphaBlend;

                emitter->setAppearance(a);
                emitter->setDampening(0.2f);
                emitter->setGlobalForce(Vector3(0, 0.2, 0.4));
                emitter->setGrowth((maxSize - size) / life * Vector2(1, 1));
                emitter->setLocalForce(Vector3(0, 0, 0));
                emitter->setRotation(Vector2(-90 / 57.3f, 90 / 57.3f));
                emitter->setSizeX(size * Vector2(0.7, 0.7));
                emitter->setSizeY(size * Vector2(0.7, 0.7));
                emitter->setSpeed(rise * Vector2(0.9f, 1));
                emitter->setSpin(Vector2(-20 / 57.3f, 20 / 57.3f));
                emitter->setSpread(Vector2(30 / 57.3f, 30 / 57.3f));

                emitter->setEmissionRate(0.15 * enabled);
                emitter->setEmitterShape(0, Box(Vector3(-1, -1, -1), Vector3(1, 1, 1)));
                emitter->setLife(life * Vector2(2, 2));

                emitter->setModulateColor(c0);
            }

            tertiary.reset();
        }
    }
    else if (Aya::Fire* fire = effect->fastDynamicCast<Aya::Fire>())
    {
        if (Aya::GameBasicSettings::singleton().getVirtualVersion() < Aya::GameBasicSettings::VERSION_2015) // 2016
        {
            float size = fire->getClampedSize() / 3.5;
            float boxsize = size / 8;
            float heat = fire->getClampedHeat();
            Vector4 c0(Color4(fire->getColor(), 1));
            Vector4 c1(Color4(fire->getSecondaryColor(), 1));
            enabled = fire->getEnabled();

            Box aabb(-boxsize * one, boxsize * one);
            Box aabb2(-boxsize * 2.f * one, boxsize * 2.f * one);

            if (enabled && !emitter)
                emitter.reset(new Emitter(getVisualEngine()));

            if (emitter)
            {
                Emitter::Appearance a = {};
                a.mainTexture = "ayaasset://textures/particles/fire_main.jxl";
                a.colorStripTexture = "ayaasset://textures/particles/fire_color.jxl";
                a.alphaStripTexture = "ayaasset://textures/particles/fire_alpha.jxl";
                a.colorStripBaseline = -1;
                a.shader = Emitter::Shader_Crazy;
                a.blendCode = Emitter::Blend_PremultipliedAlpha;

                emitter->setAppearance(a);
                emitter->setDampening(0.4);
                emitter->setGlobalForce(Vector3(0, 0, 0));
                emitter->setGrowth(0.8f * size * Vector2(-1, -1));
                emitter->setLocalForce(Vector3(0, 0.5f * (1.f * size * size / 4 + 0.7 * heat), 0));
                emitter->setRotation(Vector2(-90 / 57.3f, 90 / 57.3f));
                emitter->setSizeX(size * Vector2(1.1f, 1.1f));
                emitter->setSizeY(size * Vector2(1.1f, 1.1f));

                emitter->setSpeed(0.4f * (0.2f * size * size + 0.2f * heat) * Vector2(1, 1));
                emitter->setSpin(Vector2(100 / 57.3f, 100 / 57.3f));
                emitter->setSpread(Vector2(10 / 57.3f, 10 / 57.3f));

                emitter->setEmissionRate(65 * enabled);
                emitter->setEmitterShape(0, aabb);
                emitter->setLife(Vector2(1, 2));

                // float heatFactor = G3D::clamp( heat/25, 0.0f, 1.0f );
                // emitter->setModulateColor( G3D::lerp(3.0f, 7.0f,heatFactor) * c0 );
                emitter->setModulateColor(4.0f * c0);
                emitter->setBrightenOnThrottle(0);
                emitter->setBlendRatio(0.40f);
            }

            if (enabled && !secondary)
                secondary.reset(new Emitter(getVisualEngine()));

            if (secondary)
            {
                float size2 = 0.2f * size;
                float life2 = 2;
                float growth2 = -size2 / life2;

                Emitter::Appearance a = {};
                a.mainTexture = "ayaasset://textures/particles/fire_sparks_main.jxl";
                a.colorStripTexture = "ayaasset://textures/particles/fire_sparks_color.jxl";
                a.alphaStripTexture = "ayaasset://textures/particles/common_alpha.jxl";
                a.colorStripBaseline = -1;
                a.shader = Emitter::Shader_Add;
                a.blendCode = Emitter::Blend_Additive;

                secondary->setAppearance(a);
                secondary->setDampening(0.4);
                secondary->setGlobalForce(Vector3(0, 0, 0));
                secondary->setGrowth(growth2 * Vector2(1, 1));
                secondary->setLocalForce(Vector3(0, 0.5f * (1.f * size * size / 4 + 0.7 * heat), 0));
                secondary->setRotation(Vector2(-90 / 57.3f, 90 / 57.3f));
                secondary->setSizeX(size2 * Vector2(1, 1));
                secondary->setSizeY(size2 * Vector2(1, 1));
                secondary->setSpeed(0.4f * (0.1f * size * size + 0.2f * heat) * Vector2(1, 1));
                secondary->setSpin(Vector2(100 / 57.3f, 100 / 57.3f));
                secondary->setSpread(Vector2(10 / 57.3f, 10 / 57.3f));

                secondary->setEmissionRate(65 * enabled);
                secondary->setEmitterShape(0, aabb2);
                secondary->setLife(Vector2(life2 / 2, life2));
                secondaryOffset = CoordinateFrame(Vector3(0, 3 * (0.3 * size + 0.05 * heat), 0));

                secondary->setModulateColor(c1);
                secondary->setBrightenOnThrottle(1);
            }

            tertiary.reset();
        }
        else
        {
            // 2013
            float size = fire->getClampedSize() / 3.5;
            float boxsize = size / 12;
            float heat = fire->getClampedHeat();
            Vector4 c0(Color4(fire->getColor(), 1));
            Vector4 c1(Color4(fire->getSecondaryColor(), 1));
            enabled = fire->getEnabled();

            Box aabb(-boxsize * one, boxsize * one);
            Box aabb2(-boxsize * 2.f * one, boxsize * 2.f * one);

            if (enabled && !emitter)
                emitter.reset(new Emitter(getVisualEngine()));

            if (emitter)
            {
                Emitter::Appearance a = {};
                a.mainTexture = "ayaasset://textures/fire_0.png";
                a.colorStripTexture = "ayaasset://textures/particles/legacy_fire_alpha_color.jxl";
                a.alphaStripTexture = "ayaasset://textures/particles/legacy_fire_alpha_color.jxl";
                a.colorStripBaseline = -1;
                a.shader = Emitter::Shader_Crazy;
                a.blendCode = Emitter::Blend_PremultipliedAlpha;

                emitter->setAppearance(a);
                emitter->setDampening(0.4);
                emitter->setGlobalForce(Vector3(0, 0, 0));
                emitter->setGrowth(0.8f * size * Vector2(-1, -1));
                emitter->setLocalForce(Vector3(0, 0.5f * (1.f * size * size / 4 + 0.7 * heat), 0));
                emitter->setRotation(Vector2(-90 / 57.3f, 90 / 57.3f));
                emitter->setSizeX(size * Vector2(1.1f, 1.1f));
                emitter->setSizeY(size * Vector2(1.1f, 1.1f));

                emitter->setSpeed(0.4f * (0.2f * size * size + 0.2f * heat) * Vector2(1, 1));
                emitter->setSpin(Vector2(100 / 57.3f, 100 / 57.3f));
                emitter->setSpread(Vector2(10 / 57.3f, 10 / 57.3f));

                emitter->setEmissionRate(5 * enabled);
                emitter->setEmitterShape(0, aabb);
                emitter->setLife(Vector2(1, 1.5));

                float heatFactor = G3D::clamp(heat / 25, 0.0f, 1.0f);
                emitter->setModulateColor(G3D::lerp(3.0f, 7.0f, heatFactor) * c0);
                emitter->setModulateColor(4.0f * c0);
                emitter->setBrightenOnThrottle(0);
                emitter->setBlendRatio(0.40f);
            }

            tertiary.reset();
        }
    }
    else if (effect->isA<Aya::ForceField>())
    {
        // TODO: let's do this a better way : - )
        // kinda ugly...

        enabled = true;
        if (enabled && !emitter)
            emitter.reset(new Emitter(getVisualEngine()));

        if (Aya::GameBasicSettings::singleton().getVirtualVersion() >= Aya::GameBasicSettings::VERSION_2014)
        {
            if (emitter) // vortex
            {
                Emitter::Appearance a = {};
                a.alphaStripTexture = "ayaasset://textures/particles/common_alpha.jxl";
                a.mainTexture = "ayaasset://textures/glow.png";
                a.shader = Emitter::Shader_Crazy;
                a.blendCode = Emitter::Blend_AlphaOne;

                emitter->setAppearance(a);
                emitter->setDampening(0);
                emitter->setGlobalForce(Vector3(0, 0, 0));
                emitter->setGrowth(0.5f * Vector2(1, 1));
                emitter->setLocalForce(Vector3(0, 0, 0));
                emitter->setRotation(Vector2(0, 0));
                emitter->setSizeX(5.5f * Vector2(1, 1));
                emitter->setSizeY(5.5f * Vector2(1, 1));

                emitter->setSpeed(0.0f * Vector2(1, 1));
                emitter->setSpin(Vector2(100 / 57.3f, 100 / 57.3f));
                emitter->setSpread(Vector2(2 / 57.3f, 2 / 57.3f));

                emitter->setEmissionRate(1 * enabled);
                emitter->setEmitterShape(0, Box(Vector3(0, 0, 0), Vector3(0, 0, 0)));
                emitter->setLife(Vector2(2, 2));
                emitter->setModulateColor(Vector4(1, 1, 1, 1));
                emitterOffset = Vector3(0, -0.2f, 0);
                emitter->setZOffset(2.5f);
                emitter->setInheritMotion(1);
                emitter->setBrightenOnThrottle(1);
                emitter->setBlendRatio(0.15f);
                emitter->setLockedToLocalSpace(true);
                emitter->setLegacyEmitter(true);
            }
        }
        else
        {
            if (emitter) // glow
            {
                Emitter::Appearance a = {};
                a.alphaStripTexture = "ayaasset://textures/particles/common_alpha.jxl";
                a.colorStripTexture = "ayaasset://textures/particles/forcefield_vortex_color.jxl";
                a.mainTexture = "ayaasset://textures/particles/forcefield_vortex_main.jxl";
                a.colorStripBaseline = -1;
                a.shader = Emitter::Shader_Add;
                a.blendCode = Emitter::Blend_AlphaOne;

                emitter->setAppearance(a);
                emitter->setDampening(0);
                emitter->setGlobalForce(Vector3(0, -2, 0));
                emitter->setGrowth(1.6f * Vector2(1, 1));
                emitter->setLocalForce(Vector3(0, 5, 0));
                emitter->setRotation(Vector2(400 / 57.3f, 400 / 57.3f));
                emitter->setSizeX(Vector2(2, 2));
                emitter->setSizeY(Vector2(0.2f, 0.2f));

                emitter->setSpeed(2.0f * Vector2(1, 1));
                emitter->setSpin(Vector2(200 / 57.3f, 200 / 57.3f));
                emitter->setSpread(Vector2(10 / 57.3f, 10 / 57.3f));

                emitter->setEmissionRate(40 * enabled);
                emitter->setEmitterShape(0, Box(-0.5f * one, 0.5f * one));
                emitter->setLife(Vector2(1, 1));
                emitter->setModulateColor(Vector4(1, 1, 1, 1));
                emitterOffset = Vector3(0, -1.9f, 0);
                emitter->setInheritMotion(1);
                emitter->setLockedToLocalSpace(true);
            }
        }

        if (enabled && !secondary)
            secondary.reset(new Emitter(getVisualEngine()));

        if (Aya::GameBasicSettings::singleton().getVirtualVersion() < Aya::GameBasicSettings::VERSION_2014)
        {
            if (secondary) // stars
            {
                Emitter::Appearance a = {};
                a.alphaStripTexture = "ayaasset://textures/particles/common_alpha.jxl";
                a.colorStripTexture = "ayaasset://textures/particles/forcefield_vortex_color.jxl";
                a.mainTexture = "ayaasset://textures/sparkle.png";
                a.colorStripBaseline = -1;
                a.shader = Emitter::Shader_Add;
                a.blendCode = Emitter::Blend_AlphaOne;

                secondary->setAppearance(a);
                secondary->setDampening(0);
                secondary->setGlobalForce(Vector3(0, 0, 0));
                secondary->setGrowth(0.14f * Vector2(1, 1));
                secondary->setLocalForce(Vector3(0, 2, 0));
                secondary->setRotation(Vector2(0, 0));
                secondary->setSizeX(0.04f * Vector2(1, 1));
                secondary->setSizeY(0.04f * Vector2(1, 1));

                secondary->setSpeed(1.5f * Vector2(1, 1));
                secondary->setSpin(Vector2(-500 / 57.3f, 500 / 57.3f));
                secondary->setSpread(Vector2(30 / 57.3f, 30 / 57.3f));

                secondary->setEmissionRate(12 * enabled);
                secondary->setEmitterShape(0, Box(-0.8f * one, 0.8f * one));
                secondary->setLife(Vector2(1.3f, 1.3f));
                secondary->setModulateColor(Vector4(1, 1, 1, 1));
                secondary->setLockedToLocalSpace(true);
                secondaryOffset = Vector3(0, 0, 0);
            }
        }
        else
        {
            if (secondary) // stars
            {
                Emitter::Appearance a = {};
                a.alphaStripTexture = "ayaasset://textures/particles/common_alpha.jxl";
                a.mainTexture = "ayaasset://textures/sparkleLight.png";
                a.shader = Emitter::Shader_Add;
                a.blendCode = Emitter::Blend_AlphaOne;

                secondary->setAppearance(a);
                secondary->setDampening(0);
                secondary->setGlobalForce(Vector3(0, 0, 0));
                secondary->setGrowth(0.05f * Vector2(1, 1));
                secondary->setLocalForce(Vector3(0, 2, 0));
                secondary->setRotation(Vector2(0, 0));
                secondary->setSizeX(0.10f * Vector2(1, 1));
                secondary->setSizeY(1.50f * Vector2(1, 1));

                secondary->setSpeed(3.0f * Vector2(1, 1));
                secondary->setSpin(Vector2(-500 / 57.3f, 500 / 57.3f));
                secondary->setSpread(Vector2(30 / 57.3f, 30 / 57.3f));

                secondary->setEmissionRate(12 * enabled);
                secondary->setEmitterShape(0, Box(-0.8f * one, 0.8f * one));
                secondary->setLife(Vector2(1.3f, 1.3f));
                secondary->setModulateColor(Vector4(1, 1, 1, 1));
                secondary->setLockedToLocalSpace(true);
                secondaryOffset = Vector3(0, -1, 0);
            }
        }

        if (enabled && !tertiary)
            tertiary.reset(new Emitter(getVisualEngine()));

        if (Aya::GameBasicSettings::singleton().getVirtualVersion() < Aya::GameBasicSettings::VERSION_2014)
        {
            if (tertiary) // glow
            {
                Emitter::Appearance a = {};
                a.alphaStripTexture = "ayaasset://textures/particles/forcefield_glow_alpha.jxl";
                a.colorStripTexture = "ayaasset://textures/particles/forcefield_glow_color.jxl";
                a.mainTexture = "ayaasset://textures/particles/forcefield_glow_main.jxl";
                a.colorStripBaseline = -1;
                a.shader = Emitter::Shader_Crazy;
                a.blendCode = Emitter::Blend_PremultipliedAlpha;

                tertiary->setAppearance(a);
                tertiary->setDampening(0);
                tertiary->setGlobalForce(Vector3(0, 0, 0));
                tertiary->setGrowth(0.5f * Vector2(1, 1));
                tertiary->setLocalForce(Vector3(0, 0, 0));
                tertiary->setRotation(Vector2(0, 0));
                tertiary->setSizeX(4.4f * Vector2(1, 1));
                tertiary->setSizeY(4.4f * Vector2(1, 1));

                tertiary->setSpeed(0.0f * Vector2(1, 1));
                tertiary->setSpin(Vector2(200 / 57.3f, 200 / 57.3f));
                tertiary->setSpread(Vector2(2 / 57.3f, 2 / 57.3f));

                tertiary->setEmissionRate(15 * enabled);
                tertiary->setEmitterShape(0, Box(Vector3(0, 0, 0), Vector3(0, 0, 0)));
                tertiary->setLife(Vector2(2, 2));
                tertiary->setModulateColor(Vector4(1, 1, 1, 1));
                tertiaryOffset = Vector3(0, -0.5f, 0);
                tertiary->setZOffset(2.5f);
                tertiary->setInheritMotion(1);
                tertiary->setBrightenOnThrottle(1);
                tertiary->setBlendRatio(0.15f);
                tertiary->setLockedToLocalSpace(true);
            }
        }
        else
        {
            if (tertiary)
            {
                Emitter::Appearance a = {};
                a.alphaStripTexture = "ayaasset://textures/particles/common_alpha.jxl";
                a.mainTexture = "ayaasset://textures/sparkleLight2.png";
                a.shader = Emitter::Shader_Add;
                a.blendCode = Emitter::Blend_AlphaOne;

                tertiary->setAppearance(a);
                tertiary->setDampening(0);
                tertiary->setGlobalForce(Vector3(0, 0, 0));
                tertiary->setGrowth(0.05f * Vector2(1, 1));
                tertiary->setLocalForce(Vector3(0, 2, 0));
                tertiary->setRotation(Vector2(0, 0));
                tertiary->setSizeX(0.50f * Vector2(1, 1));
                tertiary->setSizeY(1.50f * Vector2(1, 1));

                tertiary->setSpeed(1.5f * Vector2(1, 1));
                tertiary->setSpin(Vector2(-500 / 57.3f, 500 / 57.3f));
                tertiary->setSpread(Vector2(30 / 57.3f, 30 / 57.3f));

                tertiary->setEmissionRate(12 * enabled);
                tertiary->setEmitterShape(0, Box(-0.8f * one, 0.8f * one));
                tertiary->setLife(Vector2(0.9f, 0.9f));
                tertiary->setModulateColor(Vector4(1, 1, 1, 1));
                tertiary->setLockedToLocalSpace(true);
                tertiaryOffset = Vector3(0, -1, 0);
            } // extra sparkles
        }
    }
    else if (Aya::Explosion* expl = effect->fastDynamicCast<Aya::Explosion>())
    {
        enabled = true;

        if (enabled && !emitter)
            emitter.reset(new Emitter(getVisualEngine()));

        if (Aya::GameBasicSettings::singleton().getVirtualVersion() >= Aya::GameBasicSettings::VERSION_2014)
        {
            if (emitter)
            {
                Emitter::Appearance a = {};
                a.alphaStripTexture = "ayaasset://textures/particles/explosion_alpha.jxl";
                a.colorStripTexture = "ayaasset://textures/particles/explosion_color.jxl";
                a.mainTexture = "ayaasset://textures/particles/sparkles_main.jxl";
                a.colorStripBaseline = -1;
                a.shader = Emitter::Shader_Modulate;
                a.blendCode = Emitter::Blend_AlphaOne;

                emitter->setAppearance(a);
                emitter->setEmissionRate(0);
                emitter->setGlobalForce(Vector3(0, -40, 0));
                emitter->setDampening(0.2f);
                emitter->setSpeed(Vector2(0, 30));
                emitter->setSpread(Vector2(75 / 57.3f, 75 / 57.3f));
                emitter->setSizeX(0.7f * Vector2(1, 1));
                emitter->setSizeY(0.7f * Vector2(1, 1));

                emitter->setLife(Vector2(1.8f, 2.0f));
                emitter->setEmissionRate(0);
                emitter->setEmitterShape(0, Box(0 * Vector3(-0.5f, -0.5f, -0.5f), 0 * Vector3(0.5f, 0.5f, 0.5f)));

                emitter->transform() = CoordinateFrame(expl->getPosition());
                emitter->emit(30);
            }


            if (!secondary)
                secondary.reset(new Emitter(getVisualEngine()));

            if (secondary)
            {
                Emitter::Appearance a = {};
                a.alphaStripTexture = "ayaasset://textures/particles/explosion_alpha.jxl";
                a.colorStripTexture = "ayaasset://textures/particles/explosion_color.jxl";
                a.mainTexture = "ayaasset://textures/explosion.png";
                a.colorStripBaseline = -1;
                a.shader = Emitter::Shader_Modulate;
                a.blendCode = Emitter::Blend_AlphaBlend;

                float fireSize = 4;

                secondary->setAppearance(a);
                secondary->setEmitterShape(0, Box(0 * Vector3(-0.5f, -0.5f, -0.5f), 0 * Vector3(0.5f, 0.5f, 0.5f)));
                secondary->setEmissionRate(0);
                secondary->setGlobalForce(Vector3(0, 0, 0));
                secondary->setDampening(0.5f);
                secondary->setSpeed(Vector2(15, 30));
                secondary->setSpread(Vector2(65 / 57.3f, 65 / 57.3f));
                secondary->setSpin(90 / 57.3f * Vector2(-1, 1));
                secondary->setLife(Vector2(1.5f, 2.5f));
                secondary->setSizeX(fireSize * Vector2(1, 1));
                secondary->setSizeY(fireSize * Vector2(1, 1));
                secondary->setRotation(45 / 57.3f * Vector2(-1, 1));
                secondary->setGrowth(Vector2(1.0f, 1.0f));
                secondary->setEmissionRate(0);
                secondary->setModulateColor(Vector4(0.8f, 0.5f, 0.5f, 0.5f));

                secondary->transform() = CoordinateFrame(expl->getPosition());

                secondary->emit(30);
            }

            tertiary.reset();
        }
        else
        {
            if (emitter)
            {
                Emitter::Appearance a = {};
                a.alphaStripTexture = "ayaasset://textures/particles/explosion_alpha.jxl";
                a.colorStripTexture = "ayaasset://textures/particles/explosion_color.jxl";
                a.mainTexture = "ayaasset://textures/particles/sparkles_main.jxl";
                a.colorStripBaseline = -1;
                a.shader = Emitter::Shader_Modulate;
                a.blendCode = Emitter::Blend_AlphaOne;

                emitter->setAppearance(a);
                emitter->setEmissionRate(0);
                emitter->setGlobalForce(Vector3(0, -40, 0));
                emitter->setDampening(0.2f);
                emitter->setSpeed(Vector2(0, 30));
                emitter->setSpread(Vector2(75 / 57.3f, 75 / 57.3f));
                emitter->setSizeX(0.7f * Vector2(1, 1));
                emitter->setSizeY(0.7f * Vector2(1, 1));

                emitter->setLife(Vector2(1.8f, 2.0f));
                emitter->setEmissionRate(0);
                emitter->setEmitterShape(0, Box(0 * Vector3(-0.5f, -0.5f, -0.5f), 0 * Vector3(0.5f, 0.5f, 0.5f)));

                emitter->transform() = CoordinateFrame(expl->getPosition());
                emitter->emit(30);
            }


            if (!secondary)
                secondary.reset(new Emitter(getVisualEngine()));

            if (secondary)
            {
                Emitter::Appearance a = {};
                a.alphaStripTexture = "ayaasset://textures/particles/explosion_alpha.jxl";
                a.colorStripTexture = "ayaasset://textures/particles/explosion_color.jxl";
                a.mainTexture = "ayaasset://textures/explosion.png";
                a.colorStripBaseline = -1;
                a.shader = Emitter::Shader_Modulate;
                a.blendCode = Emitter::Blend_AlphaBlend;

                float fireSize = 4;

                secondary->setAppearance(a);
                secondary->setEmitterShape(0, Box(0 * Vector3(-0.5f, -0.5f, -0.5f), 0 * Vector3(0.5f, 0.5f, 0.5f)));
                secondary->setEmissionRate(0);
                secondary->setGlobalForce(Vector3(0, 0, 0));
                secondary->setDampening(0.5f);
                secondary->setSpeed(Vector2(15, 30));
                secondary->setSpread(Vector2(65 / 57.3f, 65 / 57.3f));
                secondary->setSpin(90 / 57.3f * Vector2(-1, 1));
                secondary->setLife(Vector2(1.5f, 2.5f));
                secondary->setSizeX(fireSize * Vector2(1, 1));
                secondary->setSizeY(fireSize * Vector2(1, 1));
                secondary->setRotation(45 / 57.3f * Vector2(-1, 1));
                secondary->setGrowth(Vector2(1.0f, 1.0f));
                secondary->setEmissionRate(0);
                secondary->setModulateColor(Vector4(0.8f, 0.5f, 0.5f, 0.5f));

                secondary->transform() = CoordinateFrame(expl->getPosition());

                secondary->emit(30);
            }

            tertiary.reset();
        }
    }

    localBox = Extents();

    if (emitter)
        localBox.expandToContain(emitter->computeBBox());

    if (secondary)
        localBox.expandToContain(secondary->computeBBox());
}

} // namespace Graphics
} // namespace Aya