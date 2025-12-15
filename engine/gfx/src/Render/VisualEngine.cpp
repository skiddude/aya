
#include "Render/VisualEngine.hpp"

#include "Base/RenderCaps.hpp"
#include "Base/RenderStats.hpp"
#include "Base/FrameRateManager.hpp"

#include "Core/Device.hpp"

#include "Render/GlobalShaderData.hpp"
#include "Render/ShaderManager.hpp"
#include "Render/TextureManager.hpp"
#include "Render/TextureCompositor.hpp"
#include "Render/LightGrid.hpp"
#include "Render/Water.hpp"
#include "Render/EmitterShared.hpp"
#include "Render/MaterialGenerator.hpp"
#include "Render/SceneManager.hpp"
#include "Render/TypesetterDynamic.hpp"
#include "Render/AdornRender.hpp"
#include "Render/VertexStreamer.hpp"
#include "Render/SceneUpdater.hpp"
#include "Render/SmoothCluster.hpp"
#include "Render/TextureAtlas.hpp"

#include "Base/Typesetter.hpp"

#include "DataModel/DataModel.hpp"
#include "DataModel/ContentProvider.hpp"
#include "DataModel/MeshContentProvider.hpp"
#include "DataModel/SolidModelContentProvider.hpp"
#include "DataModel/Camera.hpp"
#include "DataModel/MegaCluster.hpp"
#include "DataModel/Workspace.hpp"
#include "DataModel/Lighting.hpp"
#include "DataModel/GameBasicSettings.hpp"
#include "SystemUtil.hpp"

#include "DataModel/UserInputService.hpp"

LOGGROUP(Graphics)

FASTFLAGVARIABLE(CancelPendingTextureLoads, true)
FASTFLAGVARIABLE(ResetTypesetterOnResize, true)

namespace Aya
{
namespace Graphics
{

VisualEngine::VisualEngine(Device* device, CRenderSettings* settings)
    : device(device)
    , viewWidth(0)
    , viewHeight(0)
    , settings(settings)
    , contentProvider(0)
    , lighting(0)
    , meshContentProvider(0)
{
    renderStats.reset(new RenderStats());
    renderCaps.reset(new RenderCaps("Unknown", SystemUtil::getVideoMemory()));

    FASTLOG1(FLog::Graphics, "Video memory size: %lld", SystemUtil::getVideoMemory());
    printf("VisualEngine::VisualEngine\n");

    GlobalShaderData::define(device);

    // load shaders
    shaderManager.reset(new ShaderManager(this));
    shaderManager->loadShaders(device->getShadingLanguage(), /* consoleOutput= */ false, false);

    // initialize texture manager
    textureManager.reset(new TextureManager(this));

    // create light grid
    LightGrid::TextureMode gridTextureMode = device->getShadingLanguage() == "glsles"                                     ? LightGrid::Texture_2D
                                             : (device->getCaps().supportsShaders && device->getCaps().supportsTexture3D) ? LightGrid::Texture_3D
                                                                                                                          : LightGrid::Texture_None;

    // Note: if there is no texture support we still create a 4x4x4 texture
    // We would really like the height to be constant since anything above the height does not cast shadows, so this affects ceiling heights
    // Having XZ size of 2x2 does not really let us have an efficient sliding window - a character is often too close to one of the edges
    // 3x3 is an option, but it's easier to go with 4x4 for now.
    Vector3int32 gridSize = (gridTextureMode != LightGrid::Texture_None && renderCaps->getVidMemSize() > 100 * 1024 * 1024)
                                ? Vector3int32(8, 4, 8) // 8x4x8 chunks take 16 Mb of VRAM
                                : Vector3int32(4, 4, 4);

    LightGrid* lgrid = LightGrid::create(this, gridSize, gridTextureMode);
    AYAASSERT(lgrid);

    lightGrid.reset(lgrid);

    classicRendering = false;

    // load fonts
    glyphAtlas.reset(new TextureAtlas(this, 2048, 2048));
    for (Text::Font font = Text::FONT_LEGACY; font != Text::FONT_LAST; font = Text::Font(font + 1))
    {
        static const char* kFontTTFPaths[] = {"fonts/arial.ttf", "fonts/arial.ttf", "fonts/arialbd.ttf", "fonts/SourceSansPro-Regular.ttf",
            "fonts/SourceSansPro-Bold.ttf", "fonts/SourceSansPro-Light.ttf", "fonts/SourceSansPro-It.ttf", "fonts/AccanthisADFStd-Regular.otf",
            "fonts/Guru-Regular.otf", "fonts/ComicNeue-Angular-Bold.ttf", "fonts/Inconsolata-Regular.ttf", "fonts/HWYGOTH.ttf", "fonts/zekton_rg.ttf",
            "fonts/PressStart2P-Regular.ttf", "fonts/Balthazar-Regular.ttf", "fonts/RomanAntique.otf", "fonts/SourceSansPro-Semibold.ttf",
            "fonts/GothamSSm-Book.otf", "fonts/GothamSSm-Medium.otf", "fonts/GothamSSm-Bold.otf", "fonts/GothamSSm-Black.otf",
            "fonts/AmaticSC-Regular.ttf", "fonts/Bangers-Regular.ttf", "fonts/Creepster-Regular.ttf", "fonts/DenkOne-Regular.ttf",
            "fonts/Fondamento-Regular.ttf", "fonts/FredokaOne-Regular.ttf", "fonts/GrenzeGotisch-Regular.ttf", "fonts/IndieFlower-Regular.ttf",
            "fonts/JosefinSans-Regular.ttf", "fonts/Jura-Regular.ttf", "fonts/Kalam-Regular.ttf", "fonts/LuckiestGuy-Regular.ttf",
            "fonts/Merriweather-Regular.ttf", "fonts/Michroma-Regular.ttf", "fonts/Nunito-Regular.ttf", "fonts/Oswald-Regular.ttf",
            "fonts/PatrickHand-Regular.ttf", "fonts/PermanentMarker-Regular.ttf", "fonts/Roboto-Regular.ttf", "fonts/RobotoCondensed-Regular.ttf",
            "fonts/RobotoMono-Regular.ttf", "fonts/Sarpanch-Regular.ttf", "fonts/SpecialElite-Regular.ttf", "fonts/TitilliumWeb-Regular.ttf",
            "fonts/Ubuntu-Regular.ttf", "fonts/NotoSans-Regular.ttf", "fonts/NotoSans-Medium.ttf", "fonts/NotoSans-SemiBold.ttf",
            "fonts/NotoSans-Bold.ttf", "fonts/NotoSans-ExtraBold.ttf", "fonts/NotoSans-Black.ttf", "fonts/NotoSans-Italic.ttf",
            "fonts/NotoSans-Light.ttf", "fonts/NotoSans-ExtraLight.ttf", "fonts/NotoSans-Thin.ttf", "fonts/CONSOLAS.ttf", "fonts/COMIC.ttf"};

        const char* ttf = kFontTTFPaths[font];

        if (Aya::GameBasicSettings::singleton().getVirtualVersion() > GameBasicSettings::VERSION_2014 && font == Text::FONT_ARIALBOLD)
        {
            ttf = "fonts/arialbd_classic.ttf";
        }

        float legacyHeightScale = (font == Text::FONT_LEGACY) ? 1.5f : 1.f;
        typesetters[font].reset(new TypesetterDynamic(glyphAtlas.get(), textureManager.get(), ContentProvider::assetFolder() + ttf, legacyHeightScale,
            (unsigned)font, device->getCaps().retina));
    }

    materialGenerator.reset(new MaterialGenerator(this));

    sceneManager.reset(new SceneManager(this));

    frameRateManager.reset(new FrameRateManager());

    vertexStreamer.reset(new VertexStreamer(this));

    textureCompositor.reset(new TextureCompositor(this));

    water.reset(new Water(this));

    emitterShared.reset(new EmitterShared);

    // set up caps
    bool gbufferSupported = shaderManager->getProgram("PassThroughVS", "SSAOFS").get() != NULL && device->getCaps().maxDrawBuffers >= 2 &&
                            (SystemUtil::getVideoMemory() >= 128 * 1024 * 1024);

    renderCaps->setSupportsGBuffer(gbufferSupported);

    if (shared_ptr<ShaderProgram> program = shaderManager->getProgram("DefaultSkinnedVS", "DefaultFS"))
    {
        unsigned int boneCount = program->getMaxWorldTransforms();

        FASTLOG1(FLog::Graphics, "Supported bones for skinning: %d", boneCount);

        renderCaps->setSkinningBoneCount(boneCount);
    }
    else
    {
        FASTLOG(FLog::Graphics, "Supported bones for skinning: 0 (no shader support)");
    }

    // configure FRM after caps are valid
    frameRateManager->Configure(renderCaps.get(), settings);
}

const shared_ptr<Typesetter>& VisualEngine::getTypesetter(Text::Font font)
{
    return typesetters[font];
}

VisualEngine::~VisualEngine()
{
    bindWorkspace(shared_ptr<DataModel>());
}

void VisualEngine::bindWorkspace(const shared_ptr<DataModel>& dm)
{
    if (dm)
    {
        contentProvider = ServiceProvider::create<ContentProvider>(dm.get());
        meshContentProvider = ServiceProvider::create<MeshContentProvider>(dm.get());
        lighting = ServiceProvider::create<Lighting>(dm.get());
        meshContentProvider->setCacheSize(settings->getMeshCacheSize());
        ServiceProvider::create<SolidModelContentProvider>(dm.get());

        AYAASSERT(!sceneUpdater);
        sceneUpdater.reset(new SceneUpdater(dm, this));

        adorn.reset(new AdornRender(this, dm.get()));

        if (lightGrid)
        {
            // Clear the grid and do an initial upload of all chunks to ensure texture has correct data
            lightGrid->lightingClearAll();
            lightGrid->lightingUploadAll();
            lightGrid->lightingUploadCommit();
        }
    }
    else
    {
        contentProvider = NULL;
        meshContentProvider = NULL;
        lighting = NULL;

        if (sceneUpdater)
        {
            sceneUpdater->unbind();
            sceneUpdater.reset();
        }

        adorn.reset();

        if (FFlag::CancelPendingTextureLoads)
        {
            textureCompositor->cancelPendingRequests();
            textureManager->cancelPendingRequests();
        }
    }
}

void VisualEngine::setViewport(int width, int height)
{
    viewWidth = width;
    viewHeight = height;
}

void VisualEngine::setCamera(const Camera& value, const G3D::Vector3& poi)
{
    camera.setViewCFrame(value.coordinateFrame(), value.getRoll());

    camera.setProjectionPerspective(value.getFieldOfView(), static_cast<float>(viewWidth) / viewHeight, -value.nearPlaneZ(), -value.farPlaneZ());

    sceneManager->setPointOfInterest(poi);

    if (sceneUpdater)
        sceneUpdater->setPointOfInterest(poi);

    // get frustum with the far clip plane to max block distance
    float updateFarPlaneZ;
    if (frameRateManager)
    {
        updateFarPlaneZ = (float)-std::max(1.0, frameRateManager->GetMaxNextViewCullDistance());
        updateFarPlaneZ = std::max(updateFarPlaneZ, value.farPlaneZ());
    }
    else
    {
        updateFarPlaneZ = value.farPlaneZ();
    }

    value.frustum(updateFarPlaneZ, updateFrustum);

    // update camera for culling
    cameraCull = camera;

    // update camera for FRM culling
    cameraCullFrm = cameraCull;

    cameraCullFrm.changeProjectionPerspectiveZ(-value.nearPlaneZ(), std::min(-value.farPlaneZ(), sqrtf(frameRateManager->GetRenderCullSqDistance())));
}

void VisualEngine::toggleClassicRendering(bool on)
{
    classicRendering = on;

    if (shaderManager)
    {
        shaderManager->toggleClassicRendering(on);
    }

    if (UserInputService* userInputService = Aya::ServiceProvider::find<UserInputService>(sceneUpdater->getDataModel().get()))
    {
        userInputService->popMouseIcon(*new TextureId(userInputService->getDefaultMouseCursor(true)));
    }
}

void VisualEngine::reloadShaders()
{
    // shaderManager->loadShaders(device->getShadingLanguage(), /* consoleOutput= */ true, true);
}

void VisualEngine::flush()
{
    textureManager->clearCache();
    materialGenerator->clearCache();
    sceneUpdater->eraseAllHumanoidClusters();
    sceneUpdater->invalidateAllFastClusters();
    textureCompositor->resetMeshCache();
    resetTypesetter();
}

void VisualEngine::flushTextures()
{
    textureManager->clearCache();
    materialGenerator->clearCache();
}
void VisualEngine::resize(int w, int h)
{
    device->resize(w, h);
}
void VisualEngine::resetTypesetter()
{
    // if (FFlag::ResetTypesetterOnResize)
    {
        // Aya::StandardOut::singleton()->printf(MESSAGE_INFO, "Resetting typesetter");

        glyphAtlas.reset(new TextureAtlas(this, 2048, 2048));
        for (Text::Font font = Text::FONT_LEGACY; font != Text::FONT_LAST; font = Text::Font(font + 1))
        {
            static const char* kFontTTFPaths[] = {"fonts/arial.ttf", "fonts/arial.ttf", "fonts/arialbd.ttf", "fonts/SourceSansPro-Regular.ttf",
                "fonts/SourceSansPro-Bold.ttf", "fonts/SourceSansPro-Light.ttf", "fonts/SourceSansPro-It.ttf", "fonts/AccanthisADFStd-Regular.otf",
                "fonts/Guru-Regular.otf", "fonts/ComicNeue-Angular-Bold.ttf", "fonts/Inconsolata-Regular.ttf", "fonts/HWYGOTH.ttf",
                "fonts/zekton_rg.ttf", "fonts/PressStart2P-Regular.ttf", "fonts/Balthazar-Regular.ttf", "fonts/RomanAntique.otf",
                "fonts/SourceSansPro-Semibold.ttf", "fonts/GothamSSm-Book.otf", "fonts/GothamSSm-Medium.otf", "fonts/GothamSSm-Bold.otf",
                "fonts/GothamSSm-Black.otf", "fonts/AmaticSC-Regular.ttf", "fonts/Bangers-Regular.ttf", "fonts/Creepster-Regular.ttf",
                "fonts/DenkOne-Regular.ttf", "fonts/Fondamento-Regular.ttf", "fonts/FredokaOne-Regular.ttf", "fonts/GrenzeGotisch-Regular.ttf",
                "fonts/IndieFlower-Regular.ttf", "fonts/JosefinSans-Regular.ttf", "fonts/Jura-Regular.ttf", "fonts/Kalam-Regular.ttf",
                "fonts/LuckiestGuy-Regular.ttf", "fonts/Merriweather-Regular.ttf", "fonts/Michroma-Regular.ttf", "fonts/Nunito-Regular.ttf",
                "fonts/Oswald-Regular.ttf", "fonts/PatrickHand-Regular.ttf", "fonts/PermanentMarker-Regular.ttf", "fonts/Roboto-Regular.ttf",
                "fonts/RobotoCondensed-Regular.ttf", "fonts/RobotoMono-Regular.ttf", "fonts/Sarpanch-Regular.ttf", "fonts/SpecialElite-Regular.ttf",
                "fonts/TitilliumWeb-Regular.ttf", "fonts/Ubuntu-Regular.ttf", "fonts/NotoSans-Regular.ttf", "fonts/NotoSans-Medium.ttf",
                "fonts/NotoSans-SemiBold.ttf", "fonts/NotoSans-Bold.ttf", "fonts/NotoSans-ExtraBold.ttf", "fonts/NotoSans-Black.ttf",
                "fonts/NotoSans-Italic.ttf", "fonts/NotoSans-Light.ttf", "fonts/NotoSans-ExtraLight.ttf", "fonts/NotoSans-Thin.ttf",
                "fonts/CONSOLAS.ttf", "fonts/COMIC.ttf"};

            const char* ttf = kFontTTFPaths[font];

            if (Aya::GameBasicSettings::singleton().getVirtualVersion() > GameBasicSettings::VERSION_2014 && font == Text::FONT_ARIALBOLD)
            {
                ttf = "fonts/arialbd_classic.ttf";
            }

            float legacyHeightScale = (font == Text::FONT_LEGACY) ? 1.5f : 1.f;
            typesetters[font].reset(new TypesetterDynamic(glyphAtlas.get(), textureManager.get(), ContentProvider::assetFolder() + ttf,
                legacyHeightScale, (unsigned)font, device->getCaps().retina));
        }
    }
}

void VisualEngine::reloadQueuedAssets()
{
    for (FilenameCountdown::iterator it = assetsToReload.begin(); it != assetsToReload.end(); ++it)
    {
        --it->second;

        if (it->second == 0)
        {
            const std::string& filePath = it->first;
            immediateAssetReload(filePath);
            assetsToReload.erase(it);

            break; // one reload per frame is enough
        }
    }
}

void VisualEngine::queueAssetReload(const std::string& filePath)
{
    if (!filePath.empty())
        assetsToReload[filePath] = 2; // wait this amount of frames before reloading
}

static void reloadMaterialTable(MegaClusterInstance* mci)
{
    mci->reloadMaterialTable();

    if (SmoothClusterBase* sc = dynamic_cast<SmoothClusterBase*>(mci->getGfxPart()))
        sc->reloadMaterialTable();
}

void VisualEngine::immediateAssetReload(const std::string& filePath)
{
    if (filePath.find("ayaasset://") == 0 || filePath.find("ayagameasset://") == 0 || filePath.find("ayaapp://") == 0 ||
        filePath.find("rbxasset://") == 0 || filePath.find("rbxgameasset://") == 0 || filePath.find("rbxapp://") == 0)
    {
        if (filePath == "ayaasset://terrain/materials.json" || filePath == "rbxasset://terrain/materials.json")
        {
            if (MegaClusterInstance* mci = Instance::fastDynamicCast<MegaClusterInstance>(sceneUpdater->getDataModel()->getWorkspace()->getTerrain()))
                DataModel::get(mci)->submitTask(boost::bind(reloadMaterialTable, mci), DataModelJob::Write);
        }

        getTextureManager()->reloadImage(ContentId(filePath));
    }
    else
    {
        std::string extension = filePath.substr(std::min(filePath.find_last_of(".") + 1, filePath.size()));
        if (extension == "hlsl" || extension == "h")
        {
            StandardOut::singleton()->printf(MESSAGE_INFO, "Reloading shaders");
            reloadShaders();
        }
    }
}

} // namespace Graphics
} // namespace Aya
