#include "Render/RenderView.hpp"

#include "Utility/FileSystem.hpp"
#include "GImage.hpp"
#include "BinaryOutput.hpp"
#include "DataModel/Lighting.hpp"
#include "DataModel/PartInstance.hpp"
#include "DataModel/Workspace.hpp"
#include "DataModel/Sky.hpp"
#include "DataModel/ContentProvider.hpp"
#include "DataModel/GameBasicSettings.hpp"
#include "DataModel/MeshContentProvider.hpp"
#include "DataModel/Camera.hpp"
#include "DataModel/TextService.hpp"
#include "DataModel/RenderHooksService.hpp"
#include "DataModel/Stats.hpp"
#include "DataModel/DataModel.hpp"
#include "Utility/StandardOut.hpp"
#include "World/World.hpp"
#include "DataModel/UserInputService.hpp"

#include "MathUtil.hpp"
#include "Render/VisualEngine.hpp"
#include "Render/VertexStreamer.hpp"
#include "Base/Typesetter.hpp"
#include "Render/Water.hpp"

#include "Render/SceneManager.hpp"
#include "Render/SceneUpdater.hpp"

#include "Render/TextureCompositor.hpp"

#include "Base/RenderStats.hpp"
#include "Base/FrameRateManager.hpp"
#include "Utility/IMetric.hpp"

#include "Render/TextureManager.hpp"
#include "Render/AdornRender.hpp"
#include "Render/LightGrid.hpp"
#include "Render/MaterialGenerator.hpp"
#include "Render/GeometryGenerator.hpp"
#include "imgui.h"

#if !defined(AYA_PLATFORM_DURANGO)
#include "Render/ObjectExporter.hpp"
#endif
#include "Render/TextureAtlas.hpp"

#include "FastLog.hpp"

#include "Render/Sky.hpp"
#include "SystemUtil.hpp"

#include "Core/Framebuffer.hpp"
#include "Core/Device.hpp"

#include "Render/SpatialHashedScene.hpp"
#include "Render/CullableSceneNode.hpp"
#include "Render/LightObject.hpp"

#include "time.hpp"

#include "Profiler.hpp"
#include "ImGui.hpp"
#include "Core/States.hpp"
#include "Render/ShaderManager.hpp"

#include "microprofile/microprofilefont.hpp"

#if defined(_WIN32) && !defined(AYA_PLATFORM_DURANGO)
#include <mmsystem.h>
#endif

LOGGROUP(ThumbnailRender)
LOGGROUP(RenderBreakdown)
LOGGROUP(RenderStatsOnLogs)
LOGGROUP(Graphics)

FASTFLAGVARIABLE(DebugAdornsDisabled, false)

FASTINTVARIABLE(OutlineBrightnessMin, 50)
FASTINTVARIABLE(OutlineBrightnessMax, 160)
FASTINTVARIABLE(OutlineThickness, 40)
FASTFLAGVARIABLE(RenderThumbModelReflectionsFix, false);

FASTINTVARIABLE(RenderShadowIntensity, 75)

FASTFLAG(FramerateVisualizerShow)
FASTFLAG(TaskSchedulerCyclicExecutive)

// Test: 0 - no throttling, 1 - adds 5ms to frame time, 2 - adds 10ms to frame time
ABTEST_NEWUSERS_VARIABLE(FrameRateThrottling)
FASTFLAG(ClientABTestingEnabled)

FASTFLAG(TypesettersReleaseResources);

FASTFLAGVARIABLE(RenderFixFog, false)

FASTFLAGVARIABLE(RenderLowLatencyLoop, false)

namespace Aya
{

void RenderView_InitModule()
{
    class RenderViewFactory : public IViewBaseFactory
    {
    public:
        RenderViewFactory()
        {
            ViewBase::RegisterFactory(CRenderSettings::OpenGL, this);
            ViewBase::RegisterFactory(CRenderSettings::BGFX, this);
        }

        ViewBase* Create(CRenderSettings::GraphicsMode mode, OSContext* context, CRenderSettings* renderSettings)
        {
            return new Graphics::RenderView(mode, context, renderSettings);
        }
    };

    static RenderViewFactory factory;
}

void RenderView_ShutdownModule() {}

} // namespace Aya

namespace Aya
{
namespace Graphics
{
double busyWaitLoop(double ms);

class FrameRateManagerStatsItem : public Aya::Stats::Item
{
    int qualityLevel;
    bool autoQuality;
    int numberOfSettles;
    double averageSwitches;
    double averageFPS;

public:
    FrameRateManagerStatsItem()
        : qualityLevel(0)
        , autoQuality(false)
        , numberOfSettles(0)
        , averageSwitches(0)
        , averageFPS(0)
    {
    }

    static shared_ptr<FrameRateManagerStatsItem> create()
    {
        shared_ptr<FrameRateManagerStatsItem> result = Aya::Creatable<Aya::Instance>::create<FrameRateManagerStatsItem>();

        result->createBoundChildItem("QualityLevel", result->qualityLevel);
        result->createBoundChildItem("AutoQuality", result->autoQuality);
        result->createBoundChildItem("NumberOfSettles", result->numberOfSettles);
        result->createBoundChildItem("AverageSwitches", result->averageSwitches);
        result->createBoundChildItem("AverageFPS", result->averageFPS);

        return result;
    }

    void updateData(FrameRateManager* frm)
    {
        Aya::FrameRateManager::Metrics metrics = frm->GetMetrics();

        qualityLevel = metrics.QualityLevel;
        autoQuality = metrics.AutoQuality;
        numberOfSettles = metrics.NumberOfSettles;
        averageSwitches = metrics.AverageSwitchesPerSettle;
        averageFPS = metrics.AverageFps;
    }
};

struct GraphicsModeTranslation
{
    CRenderSettings::GraphicsMode settingsItem;
    Device::API api;
};

// clang-format off
static const unsigned validGraphicsModes = 3;
static const GraphicsModeTranslation gGraphicsModesTranslation[validGraphicsModes] = {
    {CRenderSettings::BGFX, Device::API_BGFX},
    {CRenderSettings::OpenGL, Device::API_OpenGL}
};

static Vector2 computeCanvasSize(Device* device)
{
    if (Framebuffer* framebuffer = device->getMainFramebuffer())
    {
        int viewScale = (device->getCaps().retina) ? 2 : 1;

        return Vector2(framebuffer->getWidth(), framebuffer->getHeight()) / viewScale;
    }
    else
    {
        return Vector2();
    }
}

RenderView::RenderView(CRenderSettings::GraphicsMode graphicsMode, OSContext* context, CRenderSettings* renderSettings)
    : deltaMs(0)
    , timestampMs(0)
    , prepareTime(0)
    , totalRenderTime(0)
    , prepareAverage(15)
    , performAverage(15)
    , presentAverage(15)
    , artificialDelay(0)
    , gpuAverage(15)
    , lightingValid(false)
    , adornsEnabled(true)
    , outlinesEnabled(true)
    , fogEndCurrentFRM(10000)
    , fogEndTargetFRM(10000)
    , frameDataCallback(FrameDataCallback())
{
    FASTLOG1(FLog::ViewRbxInit, "RenderView created - %p", this);

#if defined(_WIN32) && !defined(AYA_PLATFORM_DURANGO)
    timeBeginPeriod(1);
#endif

    bool translatioFound = false;
    for (unsigned i = 0; i < validGraphicsModes; ++i)
    {
        if (gGraphicsModesTranslation[i].settingsItem == graphicsMode)
        {
            device.reset(Device::create(gGraphicsModesTranslation[i].api, context->hWnd, context->display, context->width, context->height));
            translatioFound = true;
            break;
        }
    }

    if (!translatioFound)
        throw Aya::runtime_error("Not supported graphics mode");

    const DeviceCaps& caps = device->getCaps();

    if (!caps.supportsFramebuffer)
        throw std::runtime_error("Device does not support framebuffers");

    if (!caps.supportsShaders && !caps.supportsFFP)
        throw std::runtime_error("Device does not support shaders or FFP");

    visualEngine.reset(new VisualEngine(device.get(), renderSettings));
}

void RenderView::enableAdorns(bool enable)
{
    adornsEnabled = enable;
}

void RenderView::initResources()
{
    FASTLOG1(FLog::ViewRbxInit, "RenderView::initResources - %p", this);

    FASTLOG(FLog::ViewRbxInit, "RenderView::initResources finish");
}

void RenderView::bindWorkspace(boost::shared_ptr<Aya::DataModel> dataModel)
{
    if (this->dataModel == dataModel)
        return;

    FASTLOG2(FLog::ViewRbxInit, "BindWorkspace, new datamodel %p, old datamodel: %p", dataModel.get(), this->dataModel.get());

    this->lightingValid = false;

    if (this->dataModel)
    {
        if (frameRateManagerStatsItem)
        {
            frameRateManagerStatsItem->setParent(NULL);
            frameRateManagerStatsItem.reset();
        }

        lightingChangedConnection.disconnect();

        if (TextService* textService = ServiceProvider::create<TextService>(this->dataModel.get()))
        {
            textService->clearTypesetters();
        }

        visualEngine->bindWorkspace(shared_ptr<DataModel>());
    }

    this->dataModel = dataModel;

    if (this->dataModel)
    {
        visualEngine->bindWorkspace(dataModel);

        Lighting* lighting = ServiceProvider::create<Lighting>(dataModel.get());
        lightingChangedConnection = lighting->lightingChangedSignal.connect(boost::bind(&RenderView::invalidateLighting, this, _1));

        if (TextService* textService = ServiceProvider::create<TextService>(dataModel.get()))
        {
            for (Text::Font font = Text::FONT_LEGACY; font != Text::FONT_LAST; font = Text::Font(font + 1))
            {
                if (FFlag::TypesettersReleaseResources)
                {
                    visualEngine->getTypesetter(font)->loadResources(visualEngine->getTextureManager(), visualEngine->getGlyphAtlas());
                }
                textService->registerTypesetter(TextService::FromTextFont(font), visualEngine->getTypesetter(font));
            }
        }

        FrameRateManager* frm = visualEngine->getFrameRateManager();
        if (frm)
        {
            frm->StartCapturingMetrics();

            Aya::Stats::StatsService* stats = Aya::ServiceProvider::create<Aya::Stats::StatsService>(this->dataModel.get());

            frameRateManagerStatsItem = FrameRateManagerStatsItem::create();
            frameRateManagerStatsItem->setName("FrameRateManager");
            frameRateManagerStatsItem->setParent(stats);
            frameRateManagerStatsItem->updateData(frm);
        }

        if (visualEngine->getDevice()->getMainFramebuffer())
        {
            this->dataModel->setInitialScreenSize(computeCanvasSize(visualEngine->getDevice()));
        }

        Aya::RenderHooksService* service = Aya::ServiceProvider::find<Aya::RenderHooksService>(dataModel.get());
        service->setRenderHooks(this);
        service->reloadShadersSignal.connect(boost::bind(&RenderView::reloadShaders, this));
        service->resetTypesetterSignal.connect(boost::bind(&RenderView::resetTypesetter, this));
        service->flushSignal.connect(boost::bind(&RenderView::flush, this));

        Aya::GameBasicSettings::singleton().toggleClassicRenderingSignal.connect(boost::bind(&RenderView::toggleClassicRendering, this, _1));
        // Aya::GameBasicSettings::singleton().virtualVersionChangedSignal.connect(boost::bind(&RenderView::resetTypesetter, this));

        // default fog
        fogEndCurrentFRM = fogEndTargetFRM = lighting->getFogEnd();

        visualEngine->getSettings()->setDrawConnectors(dataModel->isStudio());
    }
}


RenderView::~RenderView(void)
{
    bindWorkspace(boost::shared_ptr<Aya::DataModel>());

#if defined(_WIN32) && !defined(AYA_PLATFORM_DURANGO)
    timeEndPeriod(1);
#endif
    FASTLOG(FLog::ViewRbxInit, "RenderView destroyed");
}

void RenderView::onResize(int cx, int cy)
{
    visualEngine->resetTypesetter();
}

void RenderView::resetTypesetter()
{
    visualEngine->resetTypesetter();
}

void RenderView::resetShadowMap()
{
    visualEngine->getSceneManager()->resetShadowMap();
}

FrameRateManager* RenderView::getFrameRateManager()
{
    return visualEngine->getFrameRateManager();
}

Aya::Instance* RenderView::getWorkspace()
{
    return dataModel->getWorkspace();
}

void RenderView::invalidateLighting(bool updateSkybox)
{
    lightingValid = false;
}

void RenderView::updateFog()
{
    static const float TAU = 0.04f; // dampening factor as in:   exp( -x * TAU )

    Aya::Lighting* lighting = ServiceProvider::create<Aya::Lighting>(dataModel.get());

    if (FFlag::RenderFixFog)
    {
        float currDeltaFRM = fogEndTargetFRM - fogEndCurrentFRM;
        if (currDeltaFRM > 0)
            fogEndCurrentFRM += currDeltaFRM * TAU;
        else
            fogEndCurrentFRM = fogEndTargetFRM;

        float fogEnd = std::max(fogEndCurrentFRM, lighting->getFogEnd() * 0.5f);
        float fogStart = G3D::clamp(lighting->getFogStart(), 0.0f, fogEnd - 0.1f);

        visualEngine->getSceneManager()->setFog(lighting->getFogColor(), fogStart, fogEnd);
    }
    else
    {
        float frmEnd = this->fogEndTargetFRM;
        float fogStart = lighting->getFogStart();
        float fogEnd = lighting->getFogEnd();
        float range = fogEnd - fogStart;

        // set fog
        fogEnd = std::min(fogEnd, frmEnd);
        float currDelta = fogEnd - this->fogEndCurrentFRM;
        fogEnd = this->fogEndCurrentFRM += currDelta * TAU;
        if (frmEnd < lighting->getFogEnd() && currDelta < 0)
        {
            fogEnd = this->fogEndCurrentFRM = frmEnd; // force instant transition
        }

        fogEnd = this->fogEndCurrentFRM = std::max(fogEnd, lighting->getFogEnd() * 0.5f);
        fogStart = fogEnd - range * (fogEnd / lighting->getFogEnd());

        // Make sure fog range is not empty to avoid issues
        fogStart = G3D::clamp(fogStart, 0.0f, fogEnd - 0.1f);

        visualEngine->getSceneManager()->setFog(lighting->getFogColor(), fogStart, fogEnd);
    }
}

void RenderView::updateLighting(Lighting* lighting)
{
    SceneManager* smgr = visualEngine->getSceneManager();

    if (!lightingValid)
    {
        // Sky
        if (FFlag::RenderThumbModelReflectionsFix ||
            !lighting->isSkySuppressed()) // prepare the sky regardless of whether it's suppressed, because it's needed for the envmaps
        {
            Sky* sky = smgr->getSky();

            if (lighting->sky)
            {
                sky->setSkyBox(lighting->sky->skyRt, lighting->sky->skyLf, lighting->sky->skyBk, lighting->sky->skyFt, lighting->sky->skyUp,
                    lighting->sky->skyDn);
                sky->setCelestialBodies(lighting->sky->sunTexture, lighting->sky->moonTexture);
                sky->update(lighting->getSkyParameters(), lighting->sky->getNumStars(), lighting->sky->drawCelestialBodies,
                    lighting->sky->sunAngularSize, lighting->sky->moonAngularSize);
            }
            else
            {
                sky->setSkyBoxDefault();
                sky->setCelestialBodiesDefault();
                sky->update(lighting->getSkyParameters(), 3000, true, 21, 11);
            }

            if (lighting->blurEffect && lighting->blurEffect->isEnabled())
            {
                smgr->setBlurIntensity(lighting->blurEffect->getSize());
            }
            else
            {
                smgr->setBlurIntensity(0);
            }

            if (lighting->colorCorrectionEffect && lighting->colorCorrectionEffect->isEnabled())
            {
                smgr->setColorCorrection(lighting->colorCorrectionEffect->getBrightness(), lighting->colorCorrectionEffect->getContrast(),
                    lighting->colorCorrectionEffect->getSaturation(), lighting->colorCorrectionEffect->getTintColor());
            }
            else
            {
                smgr->setColorCorrection(0.0f, 0.0f, 0.0f, Color3::white());
            }

            if (lighting->bloomEffect && lighting->bloomEffect->isEnabled())
            {
                smgr->setBloom(lighting->bloomEffect->getIntensity(), lighting->bloomEffect->getSize(), lighting->bloomEffect->getThreshold());
            }
            else
            {
                smgr->setBloom(0.0f, 0.0f, 0.0f);
            }

            // Update clear color for time-of-day
            lighting->setClearColor(lighting->getSkyParameters().skyAmbient);
        }

        // Lighting
        presetLighting(lighting);

        lightingValid = true;
    }

    smgr->setSkyEnabled(!lighting->isSkySuppressed());
    smgr->setClearColor(Color4(lighting->getClearColor(), lighting->getClearAlpha()));
}

double RenderView::getMetricValue(const std::string& name)
{
    FrameRateManager* frm = visualEngine->getFrameRateManager();

    if (name == "Delta Between Renders")
    {
        return frm != NULL ? frm->GetFrameTimeAverage() : 0.0;
    }
    else if (name == "Total Render")
    {
        return frm != NULL ? frm->GetRenderTimeAverage() : 0.0;
    }
    else if (name == "Present Time")
    {
        return presentAverage.getStats().average;
    }
    else if (name == "GPU Delay")
    {
        return gpuAverage.getStats().average;
    }

    return -1.0;
}

std::string RenderView::getRenderStatsMetric(const std::string& name)
{
    if (name.compare(0, 15, "RenderStatsPass") == 0)
    {
        RenderPassStats* stats = (name == "RenderStatsPassScene")      ? &visualEngine->getRenderStats()->passScene
                                 : (name == "RenderStatsPassShadow")   ? &visualEngine->getRenderStats()->passShadow
                                 : (name == "RenderStatsPassUI")       ? &visualEngine->getRenderStats()->passUI
                                 : (name == "RenderStatsPass3dAdorns") ? &visualEngine->getRenderStats()->pass3DAdorns
                                 : (name == "RenderStatsPassTotal")    ? &visualEngine->getRenderStats()->passTotal
                                                                       : NULL;

        if (!stats)
            return "unknown pass";

        return Aya::format("%d (%df %dv %dp %ds)", stats->batches, stats->faces, stats->vertices, stats->passChanges, stats->stateChanges);
    }

    if (name == "RenderStatsResolution")
    {
        return Aya::format("%u x %u", visualEngine->getViewWidth(), visualEngine->getViewHeight());
    }

    if (name == "RenderStatsTimeTotal")
    {
        FrameRateManager* frm = visualEngine->getFrameRateManager();
        double totalWork = frm->GetRenderTimeAverage();

        double prepareAve = frm->GetPrepareTimeAverage();
        double performAve = performAverage.getStats().average;
        double presentAve = presentAverage.getStats().average;

        return Aya::format(
            "%.1f ms (prepare %.1f, perform %.1f, present %.1f, bonus %.1f)", totalWork, prepareAve, performAve, presentAve, artificialDelay);
    }

    if (name == "RenderStatsDelta")
    {
        FrameRateManager* frm = visualEngine->getFrameRateManager();
        double totalWork = frm->GetRenderTimeAverage();

        double prepareAve = frm->GetPrepareTimeAverage();
        double performAve = performAverage.getStats().average;
        double presentAve = presentAverage.getStats().average;
        double workAve = prepareAve + performAve + presentAve;

        double frameTime = frm->GetFrameTimeAverage();

        return Aya::format("%.1f ms (work %.1f, marshal %.1f, idle %.1f)", frameTime, workAve, workAve > totalWork ? 0.0 : totalWork - workAve,
            totalWork > frameTime ? 0.0 : frameTime - totalWork);
    }

    if (name == "RenderStatsGeometryGen")
    {
        Aya::RenderStats* stats = visualEngine->getRenderStats();
        SceneUpdater* su = visualEngine->getSceneUpdater();

        return Aya::format("fast %dc %dp mega %dc queue %dc", stats->lastFrameFast.clusters, stats->lastFrameFast.parts,
            stats->lastFrameMegaClusterChunks, int(su->getUpdateQueueSize()));
    }

    if (name == "RenderStatsClusters")
    {
        Aya::RenderStats* stats = visualEngine->getRenderStats();

        return Aya::format("fw %dc %dp; dyn %dc %dp; hum %dc %dp", stats->clusterFastFW.clusters, stats->clusterFastFW.parts,
            stats->clusterFast.clusters, stats->clusterFast.parts, stats->clusterFastHumanoid.clusters, stats->clusterFastHumanoid.parts);
    }

    if (name == "RenderStatsFRMConfig")
    {
        FrameRateManager* frm = visualEngine->getFrameRateManager();

        return Aya::format("level %d (auto %s)", frm->GetQualityLevel(),
            visualEngine->getSettings()->getQualityLevel() == CRenderSettings::QualityAuto ? "on" : "off");
    }

    if (name == "RenderStatsFRMBlocks")
    {
        FrameRateManager* frm = visualEngine->getFrameRateManager();

        return Aya::format("%d (target %d)", (int)frm->GetVisibleBlockCounter(), (int)frm->GetVisibleBlockTarget());
    }

    if (name == "RenderStatsFRMDistance")
    {
        FrameRateManager* frm = visualEngine->getFrameRateManager();

        return Aya::format("render %d view %d (...%d)", (int)sqrt(frm->GetRenderCullSqDistance()), (int)sqrt(frm->GetViewCullSqDistance()),
            frm->GetRecomputeDistanceDelay());
    }

    if (name == "RenderStatsFRMAdjust")
    {
        FrameRateManager* frm = visualEngine->getFrameRateManager();

        return Aya::format("up %d down %d backoff %d backoff avg %d", frm->GetQualityDelayUp(), frm->GetQualityDelayDown(), frm->GetBackoffCounter(),
            (int)frm->GetBackoffAverage());
    }

    if (name == "RenderStatsFRMTargetTime")
    {
        FrameRateManager* frm = visualEngine->getFrameRateManager();

        if (frm->GetQualityLevel() > 0 && frm->GetQualityLevel() < CRenderSettings::QualityLevelMax - 1)
            return Aya::format("frame %.1f render %1.f throttle %u", frm->GetTargetFrameTimeForNextLevel(), frm->GetTargetRenderTimeForNextLevel(),
                frm->getPhysicsThrottling());
        else
            return Aya::format("frame n/a render n/a throttle %u", frm->getPhysicsThrottling());
    }

    if (name == "RenderStatsTC")
    {
        TextureCompositor* tc = visualEngine->getTextureCompositor();

        const TextureCompositorConfiguration& config = tc->getConfiguration();
        const TextureCompositorStats& stats = tc->getStatistics();

        return Aya::format("%dM hq %d (%dM) lq %d (%dM) cache %d (%dM)", config.budget / 1048576, stats.liveHQCount, stats.liveHQSize / 1048576,
            stats.liveLQCount, stats.liveLQSize / 1048576, stats.orphanedCount, stats.orphanedSize / 1048576);
    }

    if (name == "RenderStatsTM")
    {
        const TextureManagerStats& stats = visualEngine->getTextureManager()->getStatistics();

        return Aya::format("queued %d live %d (%dM) cache %d (%dM)", stats.queuedCount, stats.liveCount, stats.liveSize / 1048576,
            stats.orphanedCount, stats.orphanedSize / 1048576);
    }

    if (name == "RenderStatsLightGrid")
    {
        SceneUpdater* su = visualEngine->getSceneUpdater();
        const Aya::WindowAverage<double, double>::Stats& stats = su->getLightingTimeStats();

        if (su->isLightingActive())
            return Aya::format("%d (occupancy %d, oldest: %d), %.1f ms (std: %.1f)", su->getLastLightingUpdates(), su->getLastOccupancyUpdates(),
                su->getLightOldestAge(), stats.average, sqrt(stats.variance));
        else
            return "inactive";
    }

    if (name == "RenderStatsGPU")
    {
        return Aya::format("%.1f ms (present: %.1f ms) %s", gpuAverage.getStats().average, presentAverage.getStats().average,
            visualEngine->getDevice()->getAPIName().c_str());
    }

    return "";
}

void RenderView::captureMetrics(Aya::RenderMetrics& metrics)
{
    FrameRateManager* frm = visualEngine->getFrameRateManager();

    metrics.presentTime = presentAverage.getStats().average;
    metrics.GPUDelay = gpuAverage.getStats().average;
    metrics.deltaAve = frm->GetFrameTimeAverage();

    WindowAverage<double, double>::Stats renderStats = frm->GetRenderTimeStats().getSanitizedStats();

    metrics.renderAve = renderStats.average;
    GetConfidenceInterval(renderStats.average, renderStats.variance, C90, &metrics.renderConfidenceMin, &metrics.renderConfidenceMax);
    metrics.renderStd = sqrt(renderStats.variance);
}

void RenderView::printScene() {}

bool shadersNeedReloading = false;
void RenderView::reloadShaders()
{
    shadersNeedReloading = true;
}

void RenderView::flush()
{
    visualEngine->flush();
    shadersNeedReloading = true;
}

void RenderView::toggleClassicRendering(bool on)
{
    if (visualEngine)
    {
        visualEngine->toggleClassicRendering(on);
    }
    shadersNeedReloading = true;
}

struct ProxyMetric : IMetric
{
    RenderView* view;
    IMetric* parent;

    ProxyMetric(RenderView* view, IMetric* parent)
        : view(view)
        , parent(parent)
    {
    }

    virtual std::string getMetric(const std::string& metric) const
    {
        if (metric.compare(0, 11, "RenderStats") == 0)
            return view->getRenderStatsMetric(metric);
        else
            return parent->getMetric(metric);
    }

    virtual double getMetricValue(const std::string& metric) const
    {
        if (metric.compare(0, 3, "FRM") == 0)
            return view->getFrameRateManager()->getMetricValue(metric);
        else
            return parent->getMetricValue(metric);
    }
};

void RenderView::renderPrepare(IMetric* metric)
{
    renderPrepareImpl(metric, /* updateViewport= */ true);
}

void RenderView::renderPrepareImpl(IMetric* metric, bool updateViewport)
{
    AYAPROFILER_SCOPE("Render", "Prepare");

    Timer<Time::Precise> timer;

    FrameRateManager* frm = visualEngine->getFrameRateManager();

    fogEndTargetFRM = sqrt(frm->GetRenderCullSqDistance()); // filters out a glitch when manually switching FRM level

    frm->SubmitCurrentFrame(deltaMs, totalRenderTime, prepareTime, artificialDelay);

    if (dataModel->getWorkspace())
        dataModel->getWorkspace()->renderingDistance = fogEndTargetFRM;

    if (frameRateManagerStatsItem)
        frameRateManagerStatsItem->updateData(frm);

    visualEngine->reloadQueuedAssets();

    double newtimeMs = Time::now(Time::Precise).timestampSeconds() * 1000;
    if (timestampMs != 0)
    {
        deltaMs = (newtimeMs - timestampMs);
    }
    timestampMs = newtimeMs;

    if (!visualEngine->getDevice()->validate())
        return;

    if (updateViewport)
    {
        Vector2 newViewport = computeCanvasSize(visualEngine->getDevice());

        visualEngine->setViewport(newViewport.x, newViewport.y);

        if (Camera* camera = dataModel->getWorkspace()->getCamera())
        {
            // HACK: Ideally we'd update Camera viewport in renderStep since this way renderStep has data that's one frame behind
            DataModel::scoped_write_transfer request(dataModel.get());

            camera->setViewport(newViewport);
        }
    }

    dataModel->getWorkspace()->getWorld()->setFRMThrottle(frm->getPhysicsThrottling());

    Lighting* lighting = ServiceProvider::find<Lighting>(dataModel.get());
    AYAASSERT(lighting);

    updateFog();
    visualEngine->getSceneManager()->trackLightingTimeOfDay(lighting->getGameTime());

    Workspace* workspace = dataModel->getWorkspace();
    Aya::Camera* cameraobj = workspace->getCamera();

    Vector3 poi;

    if (cameraobj->getCameraSubject())
    {
        poi = cameraobj->getCameraFocus().translation;
    }
    else
    {
        poi = cameraobj->getCameraCoordinateFrame().translation;
    }

    visualEngine->setCamera(*cameraobj, poi);

    Adorn* adorn = visualEngine->getAdorn();

    adorn->preSubmitPass();

    if (adornsEnabled && !FFlag::DebugAdornsDisabled)
    {
        // HACK: Rendering code should NOT invoke Lua code or change DM in any way. But it does (in ScreenGui & Frame objects inside SurfaceGui).
        DataModel::scoped_write_transfer request(dataModel.get());

        // do 3d adorns first (they will actually be drawn after normal 3D pass)
        dataModel->renderPass3dAdorn(adorn);

        // then do 2d adorns, after 3d adorns (so that UI text shows on top)
        ProxyMetric proxyMetric(this, metric);

        dataModel->renderPass2d(adorn, &proxyMetric);
    }

    adorn->postSubmitPass();

    updateLighting(lighting);

    float dt = frm->GetFrameTimeStats().getLatest() / 1000.f;

    visualEngine->getWater()->update(dataModel.get(), dt);

    visualEngine->getSceneUpdater()->updatePrepare(0, *visualEngine->getUpdateFrustum());

    visualEngine->getTextureCompositor()->update(poi);


    // Pass FRM configuration to shaders
    GlobalShaderData& globalShaderData = visualEngine->getSceneManager()->writeGlobalShaderData();

    float shadingDistance = std::max(frm->getShadingDistance(), 0.1f);
    float fovCoefficient = visualEngine->getCamera().getProjectionMatrix()[1][1];
    float outlineScaling = fovCoefficient * visualEngine->getViewHeight() * (10.0f / FInt::OutlineThickness) * (1 / 2.0f) / shadingDistance;

    float brightnessMulFactor = outlinesEnabled ? (FInt::OutlineBrightnessMax - FInt::OutlineBrightnessMin) / 255.0f : 0.0f;
    float brightnessAddFactor = outlinesEnabled ? FInt::OutlineBrightnessMin / 255.0f : 1.0f;

    float shadowFade = powf(G3D::clamp(lighting->getSkyParameters().lightDirection.unit().y, 0.f, 1.f), 1.f / 4);
    float shadowIntensity = shadowFade * (FInt::RenderShadowIntensity / 100.f);

    globalShaderData.FadeDistance_GlowFactor =
        Vector4(shadingDistance, 1.f / shadingDistance, outlineScaling, globalShaderData.FadeDistance_GlowFactor.w);
    globalShaderData.OutlineBrightness_ShadowInfo = Vector4(brightnessMulFactor, brightnessAddFactor, 0, shadowIntensity);

    FASTLOG(FLog::ViewRbxBase, "Render prepare end");
    prepareTime = timer.delta().msec();
    prepareAverage.sample(prepareTime);
}

class ProfilerRenderer : public Profiler::Renderer
{
public:
    ProfilerRenderer(VisualEngine* visualEngine)
        : visualEngine(visualEngine)
        , colorOrderBGR(false)
    {
        Device* device = visualEngine->getDevice();

        colorOrderBGR = device->getCaps().colorOrderBGR;

        std::vector<VertexLayout::Element> elements;
        elements.push_back(VertexLayout::Element(0, offsetof(Vertex, x), VertexLayout::Format_Float2, VertexLayout::Semantic_Position));
        elements.push_back(VertexLayout::Element(0, offsetof(Vertex, u), VertexLayout::Format_Float2, VertexLayout::Semantic_Texture));
        elements.push_back(VertexLayout::Element(0, offsetof(Vertex, color), VertexLayout::Format_Color, VertexLayout::Semantic_Color));

        layout = device->createVertexLayout(elements);

        const size_t fontSize = g_MicroProfileFontTextureX * g_MicroProfileFontTextureY * 4;

        uint32_t* pUnpacked = (uint32_t*)alloca(fontSize);

        int idx = 0;
        int end = g_MicroProfileFontTextureX * g_MicroProfileFontTextureY / 8;
        for (int i = 0; i < end; i++)
        {
            unsigned char pValue = g_MicroProfileFont[i];
            for (int j = 0; j < 8; ++j)
            {
                pUnpacked[idx++] = (pValue & 0x80) ? 0xffffffff : 0;
                pValue <<= 1;
            }
        }

        pUnpacked[idx - 1] = 0xffffffff;

        fontTexture = device->createTexture(
            Texture::Type_2D, Texture::Format_RGBA8, g_MicroProfileFontTextureX, g_MicroProfileFontTextureY, 1, 1, Texture::Usage_Static);

        fontTexture->upload(0, 0, TextureRegion(0, 0, g_MicroProfileFontTextureX, g_MicroProfileFontTextureY), pUnpacked, fontSize);
    }

    void drawText(int x, int y, unsigned int color, const char* text, unsigned int length, unsigned int textWidth, unsigned int textHeight) override
    {
        const float fOffsetU = float(textWidth) / float(g_MicroProfileFontTextureX);

        float fX = (float)x;
        float fY = (float)y;
        float fY2 = fY + (textHeight + 1);

        const char* pStr = text;

        if (!colorOrderBGR)
            color = 0xff000000 | ((color & 0xff) << 16) | (color & 0xff00) | ((color >> 16) & 0xff);

        for (uint32_t j = 0; j < length; ++j)
        {
            int16_t nOffset = g_MicroProfileFontDescription[uint8_t(*pStr++)];
            float fOffset = float(nOffset) / float(g_MicroProfileFontTextureX);

            Vertex v0 = {fX, fY, fOffset, 0.f, color};
            Vertex v1 = {fX + textWidth, fY, fOffset + fOffsetU, 0.f, color};
            Vertex v2 = {fX + textWidth, fY2, fOffset + fOffsetU, 1.f, color};
            Vertex v3 = {fX, fY2, fOffset, 1.f, color};

            quads.push_back(v0);
            quads.push_back(v1);
            quads.push_back(v3);
            quads.push_back(v1);
            quads.push_back(v2);
            quads.push_back(v3);

            fX += textWidth + 1;
        }
    }

    void drawBox(int x0, int y0, int x1, int y1, unsigned int color0, unsigned int color1) override
    {
        if (!colorOrderBGR)
        {
            color0 = ((color0 & 0xff) << 16) | ((color0 >> 16) & 0xff) | (0xff00ff00 & color0);
            color1 = ((color1 & 0xff) << 16) | ((color1 >> 16) & 0xff) | (0xff00ff00 & color1);
        }

        Vertex v0 = {(float)x0, (float)y0, 2.f, 2.f, color0};
        Vertex v1 = {(float)x1, (float)y0, 2.f, 2.f, color0};
        Vertex v2 = {(float)x1, (float)y1, 2.f, 2.f, color1};
        Vertex v3 = {(float)x0, (float)y1, 2.f, 2.f, color1};

        quads.push_back(v0);
        quads.push_back(v1);
        quads.push_back(v3);
        quads.push_back(v1);
        quads.push_back(v2);
        quads.push_back(v3);
    }

    void drawLine(unsigned int vertexCount, const float* vertexData, unsigned int color) override
    {
        if (!colorOrderBGR)
            color = ((color & 0xff) << 16) | ((color >> 16) & 0xff) | (0xff00ff00 & color);

        for (unsigned int i = 0; i + 1 < vertexCount; ++i)
        {
            Vertex v0 = {vertexData[2 * i + 0], vertexData[2 * i + 1], 2.f, 2.f, color};
            Vertex v1 = {vertexData[2 * i + 2], vertexData[2 * i + 3], 2.f, 2.f, color};

            lines.push_back(v0);
            lines.push_back(v1);
        }
    }

    void flush(DeviceContext* context, const RenderCamera& camera)
    {
        if (quads.empty() && lines.empty())
            return;

        updateBuffer(quadsVB, quadsGeometry, quads);
        updateBuffer(linesVB, linesGeometry, lines);

        if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram("ProfilerVS", "ProfilerFS"))
        {
            GlobalShaderData shaderData = visualEngine->getSceneManager()->readGlobalShaderData();

            shaderData.setCamera(camera);

            context->updateGlobalConstants(&shaderData, sizeof(GlobalShaderData));

            context->bindProgram(program.get());

            context->setRasterizerState(RasterizerState::Cull_None);
            context->setBlendState(BlendState::Mode_AlphaBlend);
            context->setDepthState(DepthState(DepthState::Function_Always, false));

            context->bindTexture(0, fontTexture.get(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));

            if (!quads.empty())
                context->draw(quadsGeometry.get(), Geometry::Primitive_Triangles, 0, quads.size(), 0, quads.size());

            if (!lines.empty())
                context->draw(linesGeometry.get(), Geometry::Primitive_Lines, 0, lines.size(), 0, lines.size());
        }

        quads.clear();
        lines.clear();
    }

private:
    struct Vertex
    {
        float x, y;
        float u, v;
        unsigned int color;
    };

    VisualEngine* visualEngine;

    bool colorOrderBGR;

    std::vector<Vertex> quads;
    std::vector<Vertex> lines;

    shared_ptr<Texture> fontTexture;

    shared_ptr<VertexLayout> layout;

    shared_ptr<VertexBuffer> quadsVB;
    shared_ptr<Geometry> quadsGeometry;

    shared_ptr<VertexBuffer> linesVB;
    shared_ptr<Geometry> linesGeometry;

    void updateBuffer(shared_ptr<VertexBuffer>& vb, shared_ptr<Geometry>& geometry, const std::vector<Vertex>& vertices)
    {
        if (vertices.empty())
            return;

        if (!vb || vb->getElementCount() < vertices.size())
        {
            size_t count = 1;
            while (count < vertices.size())
                count *= 2;

            vb = visualEngine->getDevice()->createVertexBuffer(sizeof(Vertex), count, GeometryBuffer::Usage_Dynamic);
            geometry = visualEngine->getDevice()->createGeometry(layout, vb, shared_ptr<IndexBuffer>());
        }

        void* locked = vb->lock(VertexBuffer::Lock_Discard);
        memcpy(locked, vertices.data(), vertices.size() * sizeof(Vertex));
        vb->unlock();
    }
};

void RenderView::drawProfiler(DeviceContext* context)
{
    if (!Profiler::isVisible())
        return;

    if (!profilerRenderer)
        profilerRenderer.reset(new ProfilerRenderer(visualEngine.get()));

    AYAPROFILER_SCOPE("GPU", "Profiler");

    Framebuffer* fb = visualEngine->getDevice()->getMainFramebuffer();

    Profiler::render(profilerRenderer.get(), fb->getWidth(), fb->getHeight());

    RenderCamera camera;
    camera.setViewMatrix(Matrix4::identity());
    camera.setProjectionOrtho(fb->getWidth(), fb->getHeight(), -1, 1, visualEngine->getDevice()->getCaps().needsHalfPixelOffset);

    profilerRenderer->flush(context, camera);
}

class ImGuiRenderer : public ImGui::Renderer
{
public:
    ImGuiRenderer(VisualEngine* visualEngine)
        : visualEngine(visualEngine)
        , colorOrderBGR(false)
    {
        Device* device = visualEngine->getDevice();

        colorOrderBGR = device->getCaps().colorOrderBGR;

        std::vector<VertexLayout::Element> elements;
        elements.push_back(VertexLayout::Element(0, offsetof(Vertex, x), VertexLayout::Format_Float2, VertexLayout::Semantic_Position));
        elements.push_back(VertexLayout::Element(0, offsetof(Vertex, u), VertexLayout::Format_Float2, VertexLayout::Semantic_Texture));
        elements.push_back(VertexLayout::Element(0, offsetof(Vertex, color), VertexLayout::Format_Color, VertexLayout::Semantic_Color));

        layout = device->createVertexLayout(elements);

        unsigned char* pixels;
        int w, h;
        ImGui::getFont(&pixels, &w, &h);

        const size_t fontSize = w * h * 4;

        fontTexture = device->createTexture(Texture::Type_2D, Texture::Format_RGBA8, w, h, 1, 1, Texture::Usage_Static);
        fontTexture->upload(0, 0, TextureRegion(0, 0, w, h), pixels, fontSize);
    }

    void draw(ImDrawData* data)
    {
        if (!data || !data->Valid)
            return;

        // Clear previous draw commands
        drawCommands.clear();

        for (int n = 0; n < data->CmdListsCount; n++)
        {
            const ImDrawList* cmdList = data->CmdLists[n];
            
            if (!cmdList || !cmdList->VtxBuffer.Data || !cmdList->IdxBuffer.Data)
                continue;
            
            // Convert ImDrawVert to our Vertex format
            std::vector<Vertex> vertices;
            vertices.reserve(cmdList->VtxBuffer.Size);
            
            for (int i = 0; i < cmdList->VtxBuffer.Size; i++)
            {
                const ImDrawVert& v = cmdList->VtxBuffer[i];
                unsigned int col = colorOrderBGR ? convertColor(v.col) : v.col;
                vertices.push_back({v.pos.x, v.pos.y, v.uv.x, v.uv.y, col});
            }
            
            // Process each draw command
            int idxOffset = 0;
            for (int cmdI = 0; cmdI < cmdList->CmdBuffer.Size; cmdI++)
            {
                const ImDrawCmd* pcmd = &cmdList->CmdBuffer[cmdI];
                
                if (pcmd->UserCallback)
                {
                    pcmd->UserCallback(cmdList, pcmd);
                }
                else
                {
                    DrawCommand cmd;
                    cmd.texture = pcmd->TextureId ? (Texture*)pcmd->TextureId : fontTexture.get();
                    cmd.clipRect = pcmd->ClipRect;
                    
                    // Build and clip triangles for this draw command
                    for (unsigned int i = 0; i < pcmd->ElemCount; i += 3)
                    {
                        if (i + 2 >= pcmd->ElemCount)
                            break;
                        
                        ImDrawIdx idx0 = cmdList->IdxBuffer[idxOffset + i + 0];
                        ImDrawIdx idx1 = cmdList->IdxBuffer[idxOffset + i + 1];
                        ImDrawIdx idx2 = cmdList->IdxBuffer[idxOffset + i + 2];
                        
                        if (idx0 >= vertices.size() || idx1 >= vertices.size() || idx2 >= vertices.size())
                            continue;
                        
                        Vertex v0 = vertices[idx0];
                        Vertex v1 = vertices[idx1];
                        Vertex v2 = vertices[idx2];
                        
                        // Clip triangle against scissor rect
                        if (clipTriangle(v0, v1, v2, pcmd->ClipRect))
                        {
                            cmd.triangles.push_back(v0);
                            cmd.triangles.push_back(v1);
                            cmd.triangles.push_back(v2);
                        }
                    }
                    
                    // Only add command if it has triangles
                    if (!cmd.triangles.empty())
                    {
                        drawCommands.push_back(std::move(cmd));
                    }
                }
                
                idxOffset += pcmd->ElemCount;
            }
        }
    }

    void flush(DeviceContext* context, const RenderCamera& camera)
    {
        if (drawCommands.empty())
            return;

        if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram("ProfilerVS", "ImGuiFS"))
        {
            GlobalShaderData shaderData = visualEngine->getSceneManager()->readGlobalShaderData();
            shaderData.setCamera(camera);
            
            context->updateGlobalConstants(&shaderData, sizeof(GlobalShaderData));
            context->bindProgram(program.get());
            
            context->setRasterizerState(RasterizerState::Cull_None);
            context->setBlendState(BlendState::Mode_AlphaBlend);
            context->setDepthState(DepthState(DepthState::Function_Always, false));
            
            // Render each draw command with its own texture
            for (const DrawCommand& cmd : drawCommands)
            {
                if (cmd.triangles.empty())
                    continue;
                
                // Update buffer with triangles for this command
                updateBuffer(quadsVB, quadsGeometry, cmd.triangles);
                
                // Bind the appropriate texture
                context->bindTexture(0, cmd.texture, 
                    SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));
                
                // Draw triangles
                context->draw(quadsGeometry.get(), Geometry::Primitive_Triangles, 
                             0, cmd.triangles.size(), 0, cmd.triangles.size());
            }
        }

        drawCommands.clear();
    }

private:
    struct Vertex
    {
        float x, y;
        float u, v;
        unsigned int color;
    };

    struct DrawCommand
    {
        std::vector<Vertex> triangles;
        Texture* texture;
        ImVec4 clipRect;
    };

    VisualEngine* visualEngine;
    bool colorOrderBGR;

    shared_ptr<VertexLayout> layout;
    shared_ptr<VertexBuffer> quadsVB;
    shared_ptr<Geometry> quadsGeometry;
    shared_ptr<Texture> fontTexture;

    std::vector<DrawCommand> drawCommands;

    unsigned int convertColor(unsigned int color)
    {
        // ImGui uses ABGR, convert to ARGB for BGR systems
        return ((color & 0xFF00FF00)) |           // Keep A and G
               ((color & 0x000000FF) << 16) |     // B to R position
               ((color & 0x00FF0000) >> 16);      // R to B position
    }

    bool clipTriangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, const ImVec4& clipRect)
    {
        // Simple bounding box test
        float minX = std::min({v0.x, v1.x, v2.x});
        float maxX = std::max({v0.x, v1.x, v2.x});
        float minY = std::min({v0.y, v1.y, v2.y});
        float maxY = std::max({v0.y, v1.y, v2.y});
        
        // If triangle is completely outside clip rect, reject it
        if (maxX < clipRect.x || minX > clipRect.z ||
            maxY < clipRect.y || minY > clipRect.w)
        {
            return false;
        }
        
        // If triangle is completely inside or partially inside, accept it
        // Note: Triangles that cross the boundary won't be perfectly clipped,
        // but this is usually good enough for ImGui
        return true;
    }

    void updateBuffer(shared_ptr<VertexBuffer>& vb, shared_ptr<Geometry>& geometry, const std::vector<Vertex>& vertices)
    {
        if (vertices.empty())
            return;

        if (!vb || vb->getElementCount() < vertices.size())
        {
            size_t count = 1;
            while (count < vertices.size())
                count *= 2;

            vb = visualEngine->getDevice()->createVertexBuffer(sizeof(Vertex), count, GeometryBuffer::Usage_Dynamic);
            geometry = visualEngine->getDevice()->createGeometry(layout, vb, shared_ptr<IndexBuffer>());
        }

        void* locked = vb->lock(VertexBuffer::Lock_Discard);
        memcpy(locked, vertices.data(), vertices.size() * sizeof(Vertex));
        vb->unlock();
    }
};

void RenderView::drawImGui(DeviceContext* context)
{
    if (!imGuiRenderer)
        imGuiRenderer.reset(new ImGuiRenderer(visualEngine.get()));
    else
    {
        AYAPROFILER_SCOPE("GPU", "ImGui");

        Framebuffer* fb = visualEngine->getDevice()->getMainFramebuffer();

        ImGui::render(imGuiRenderer.get(), fb->getWidth(), fb->getHeight());

        RenderCamera camera;
        camera.setViewMatrix(Matrix4::identity());
        camera.setProjectionOrtho(fb->getWidth(), fb->getHeight(), -1, 1, visualEngine->getDevice()->getCaps().needsHalfPixelOffset);

        imGuiRenderer->flush(context, camera);
    }
}

void RenderView::renderPerform(double timeJobStart)
{
    renderPerformImpl(timeJobStart, visualEngine->getDevice()->getMainFramebuffer());
}

void RenderView::renderPerformImpl(double timeJobStart, Framebuffer* mainFramebuffer)
{
    AYAPROFILER_SCOPE("Render", "Perform");

    if (!this->dataModel)
        return;

    Timer<Time::Precise> timer;

    FASTLOG(FLog::ViewRbxBase, "Render perform start");

    double performTime = 0;
    double presentTime = 0;

    Adorn* adorn = visualEngine->getAdorn();

    // update glyph atlas
    visualEngine->getGlyphAtlas()->upload();

    adorn->prepareRenderPass();

    if (DeviceContext* context = visualEngine->getDevice()->beginFrame())
    {
        visualEngine->getSceneUpdater()->updatePerform();

        visualEngine->getMaterialGenerator()->garbageCollectIncremental();
        visualEngine->getTextureManager()->garbageCollectIncremental();

        visualEngine->getTextureManager()->processPendingRequests();

        context->setDefaultAnisotropy(std::max(1, visualEngine->getFrameRateManager()->getTextureAnisotropy()));

        visualEngine->getTextureCompositor()->render(context);

        visualEngine->getSceneManager()->renderScene(context, mainFramebuffer, visualEngine->getCamera(), visualEngine->getViewWidth(), visualEngine->getViewHeight());

        performTime = timer.reset().msec();
        performAverage.sample(performTime);

        drawProfiler(context);
        drawImGui(context);

        AYAPROFILER_SCOPE("Render", "Present");

        visualEngine->getDevice()->endFrame();

        presentTime = timer.delta().msec();
        presentAverage.sample(presentTime);

        gpuAverage.sample(visualEngine->getDevice()->getStatistics().gpuFrameTime);
    }

    adorn->finishRenderPass();

    totalRenderTime = (Time::nowFastSec() - timeJobStart) * 1000.0;

    if (shadersNeedReloading)
    {
        if (visualEngine)
        {
            visualEngine->getTextureManager()->clearCache();
            visualEngine->getMaterialGenerator()->clearCache();
            visualEngine->getSceneUpdater()->eraseAllHumanoidClusters();
            visualEngine->getSceneUpdater()->invalidateAllFastClusters();
            visualEngine->getTextureCompositor()->resetMeshCache();
            visualEngine->getSceneManager()->resetShadowMap(); // does this even do shit
            visualEngine->resetTypesetter();
        }
        this->invalidateLighting(true);
        shadersNeedReloading = false;
    }
}

void RenderView::buildGui(bool buildInGameGui)
{
    dataModel->startCoreScripts(buildInGameGui);
}

RenderStats& RenderView::getRenderStats()
{
    return *visualEngine->getRenderStats();
}

void RenderView::presetLighting(Aya::Lighting* l, const Aya::Color3& extraAmbient, float skylightFactor)
{
    outlinesEnabled = l->getOutlines();

    const G3D::LightingParameters& skyParameters = l->getSkyParameters();
    SceneManager* smgr = visualEngine->getSceneManager();

    Color3 bottomColorShift = l->getBottomColorShift();
    Color3 topColorShift = l->getTopColorShift();

    float bottomShiftBase = (bottomColorShift.r + bottomColorShift.g + bottomColorShift.b) * 0.3333f;
    Color3 bottomAdjustment = bottomColorShift - Color3(bottomShiftBase, bottomShiftBase, bottomShiftBase);

    float topShiftBase = (topColorShift.r + topColorShift.g + topColorShift.b) * 0.3333f;
    Color3 topAdjustment = topColorShift - Color3(topShiftBase, topShiftBase, topShiftBase);

    float globalBrightness = G3D::clamp(l->getGlobalBrightness(), 0.05f, 5.0f);

    Vector3 sunDirection = -skyParameters.lightDirection.unit();
    Color3 sunColor = skyParameters.lightColor * 0.9f;

    Color3 ambientColor = l->getGlobalAmbient() + extraAmbient;

    Color3 keyLightColor = sunColor * globalBrightness;
    keyLightColor += topAdjustment * keyLightColor.length();
    keyLightColor *= skylightFactor;

    Color3 fillLightColor = sunColor * globalBrightness * 0.4f;
    fillLightColor += bottomAdjustment * fillLightColor.length();
    fillLightColor *= skylightFactor;

    smgr->setLighting(ambientColor, sunDirection, keyLightColor.min(Color3::white()), fillLightColor.min(Color3::white()));
}

static void waitForContent(Aya::ContentProvider* contentProvider)
{
    boost::xtime expirationTime;
    boost::xtime_get(&expirationTime, boost::TIME_UTC_);
    expirationTime.sec += static_cast<boost::xtime::xtime_sec_t>(120);

    // TODO: Bail out after a certain number of seconds...
    while (!contentProvider->isRequestQueueEmpty())
    {
        boost::xtime xt;
        boost::xtime_get(&xt, boost::TIME_UTC_);

        if (xtime_cmp(xt, expirationTime) == 1)
            throw Aya::runtime_error("Timeout while waiting for content - 120 seconds");

        boost::this_thread::sleep(boost::posix_time::milliseconds(20));
    }
}

static void modifyThumbnailCamera(VisualEngine* visualEngine, bool allowDolly, int recdepth = 1)
{
    if (recdepth > 10)
        return;

    std::vector<CullableSceneNode*> nodes = visualEngine->getSceneManager()->getSpatialHashedScene()->getAllNodes();

    Extents extents;

    for (size_t i = 0; i < nodes.size(); ++i)
        if (!dynamic_cast<LightObject*>(nodes[i]))
            extents.expandToContain(nodes[i]->getWorldBounds());

    Matrix4 viewProj = visualEngine->getCamera().getViewProjectionMatrix();

    Extents screen;

    for (unsigned i = 0; i < 8; ++i)
    {
        Vector4 p = viewProj * Vector4(extents.getCorner(i), 1);
        Vector3 q = p.xyz() / p.w;

        if (p.w <= 0.001f || fabsf(q.x) > 1 || fabsf(q.y) > 1 || fabsf(q.z) > 1)
        {
            if (!allowDolly)
                return;

            Matrix4 view = visualEngine->getCameraMutable().getViewMatrix();
            Matrix4 dolly = Matrix4::translation(0, 0, -1 - recdepth * recdepth);
            visualEngine->getCameraMutable().setViewMatrix(dolly * view);
            return modifyThumbnailCamera(visualEngine, true, recdepth + 1); // try once again
        }

        screen.expandToContain(q);
    }

    float zoomH = 2 / (screen.max().x - screen.min().x);
    float zoomV = 2 / (screen.max().y - screen.min().y);
    float zoom = std::min(zoomV, zoomH);

    float offH = screen.center().x;
    float offV = screen.center().y;

    Matrix4 crop = Matrix4::identity();

    crop[0][0] = zoom;
    crop[1][1] = zoom;
    crop[0][3] = -zoom * offH;
    crop[1][3] = -zoom * offV;

    visualEngine->getCameraMutable().setProjectionMatrix(crop * visualEngine->getCamera().getProjectionMatrix());
}

void RenderView::prepareSceneGraph()
{
    // will populate scenegraph, and trigger resource loads.
    visualEngine->getSceneUpdater()->setComputeLightingEnabled(false);

    // wait for all networked resources to load.
    // do this outside scoped lock because this is the IO/network bound task.

    Aya::ContentProvider* contentProvider = visualEngine->getContentProvider();
    Aya::MeshContentProvider* meshContentProvider = visualEngine->getMeshContentProvider();
    bool allContentLoaded = false;

    do
    {
        FASTLOG(FLog::ThumbnailRender, "Waiting for content to load");

        waitForContent(contentProvider);

        Aya::ServiceProvider::find<Aya::Workspace>(dataModel.get())->assemble();

        renderPrepareImpl(NULL, /* updateViewport= */ false);

        // Clear adorn rendering queue
        visualEngine->getAdorn()->finishRenderPass();

        // Load textures
        while (!visualEngine->getTextureManager()->isQueueEmpty())
        {
            visualEngine->getTextureManager()->processPendingRequests();

            boost::this_thread::sleep(boost::posix_time::milliseconds(20));
        }

        // Run texture compositor
        if (!visualEngine->getTextureCompositor()->isQueueEmpty())
        {
            if (DeviceContext* context = visualEngine->getDevice()->beginFrame())
            {
                visualEngine->getTextureCompositor()->render(context);
                visualEngine->getDevice()->endFrame();
            }
        }

        allContentLoaded = contentProvider->isRequestQueueEmpty() && meshContentProvider->isRequestQueueEmpty() &&
                           !visualEngine->getSceneUpdater()->arePartsWaitingForAssets() && visualEngine->getTextureCompositor()->isQueueEmpty() &&
                           visualEngine->getTextureManager()->isQueueEmpty();

        FASTLOG2(FLog::ThumbnailRender, "After render: Providers requests empty. Content: %u, Mesh: %u", contentProvider->isRequestQueueEmpty(),
            meshContentProvider->isRequestQueueEmpty());

        FASTLOG3(FLog::ThumbnailRender, "Parts not waiting for assets: %u, Texture Compositor Queue Empty: %u, Texture Manager Queue Empty: %u",
            !visualEngine->getSceneUpdater()->arePartsWaitingForAssets(), visualEngine->getTextureCompositor()->isQueueEmpty(),
            visualEngine->getTextureManager()->isQueueEmpty());
    } while (!allContentLoaded);

    visualEngine->getSceneUpdater()->setComputeLightingEnabled(true);
}

void RenderView::renderThumb(unsigned char* data, int width, int height, bool crop, bool allowDolly)
{
    // will populate scenegraph, and trigger resource loads.
    FASTLOG(FLog::ThumbnailRender, "Rendering thumbnail: populating scene graph, trigger resource load");
    prepareSceneGraph();

    visualEngine->getFrameRateManager()->SubmitCurrentFrame(1000, 1000, 1000, 0);

    // Run find visible objects once to fill in distance for LightGrid
    visualEngine->getSceneManager()->computeMinimumSqDistance(visualEngine->getCamera());

    visualEngine->setViewport(width, height);

    if (Camera* camera = dataModel->getWorkspace()->getCamera())
        camera->setViewport(Vector2int16(width, height));

    renderPrepareImpl(NULL, /* updateViewport= */ false);

    if (crop)
    {
        modifyThumbnailCamera(visualEngine.get(), allowDolly);
    }

    shared_ptr<Renderbuffer> color = visualEngine->getDevice()->createRenderbuffer(Texture::Format_RGBA8, width, height, 1);
    shared_ptr<Renderbuffer> depth = visualEngine->getDevice()->createRenderbuffer(Texture::Format_D24S8, width, height, 1);
    shared_ptr<Framebuffer> framebuffer = visualEngine->getDevice()->createFramebuffer(color, depth);

    renderPerformImpl(0.f, framebuffer.get());

    framebuffer->download(data, width * height * 4);
}

void RenderView::queueAssetReload(const std::string& filePath)
{
    visualEngine->queueAssetReload(filePath);
}

void RenderView::immediateAssetReload(const std::string& filePath)
{
    visualEngine->immediateAssetReload(filePath);
}

bool RenderView::exportSceneThumbJSON(ExporterSaveType saveType, ExporterFormat format, bool encodeBase64, std::string& strOut)
{
#if !defined(AYA_PLATFORM_DURANGO)
    FASTLOG(FLog::ThumbnailRender, "Rendering thumbnail: populating scene graph, trigger resource load");
    prepareSceneGraph();

    return ObjectExporter::exportToJSON(saveType, format, dataModel.get(), visualEngine.get(), encodeBase64, &strOut);
#else
    return false;
#endif
}

bool RenderView::exportScene(const std::string& filePath, ExporterSaveType saveType, ExporterFormat format)
{
#if !defined(AYA_PLATFORM_DURANGO)
    return ObjectExporter::exportToFile(filePath, saveType, format, dataModel.get(), visualEngine.get());
#else
    return false;
#endif
}


void RenderView::suspendView()
{
    device->suspend();
}

void RenderView::resumeView()
{
    device->resume();
}

void RenderView::garbageCollect()
{
    visualEngine->getMaterialGenerator()->garbageCollectFull();
    visualEngine->getTextureCompositor()->garbageCollectFull();
    visualEngine->getTextureManager()->garbageCollectFull();

    if (Aya::ContentProvider* contentProvider = visualEngine->getContentProvider())
    {
        contentProvider->clearContent();
    }

    if (Aya::MeshContentProvider* meshContentProvider = visualEngine->getMeshContentProvider())
    {
        meshContentProvider->clearContent();
    }
}

std::pair<unsigned, unsigned> RenderView::setFrameDataCallback(const boost::function<void(void*)>& callback)
{
    Framebuffer* framebuffer = visualEngine->getDevice()->getMainFramebuffer();
    if (callback && framebuffer)
    {
        frameDataCallback = callback;
        return std::make_pair(framebuffer->getWidth(), framebuffer->getHeight());
    }
    else
    {
        frameDataCallback = FrameDataCallback();
        return std::make_pair(0, 0);
    }
}

double busyWaitLoop(double ms)
{
    double start = Time::now(Time::Precise).timestampSeconds() * 1000;
    for (;;)
    {
        double cur = Time::now(Time::Precise).timestampSeconds() * 1000;
        if (cur - start > ms)
            return cur - start;
    }
}

} // namespace Graphics
} // namespace Aya
