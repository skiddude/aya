


#include "Base/RenderStats.hpp"
#include "Utility/Profiling.hpp"

using namespace Aya;

RenderStats::RenderStats()
    : cpuRenderTotal(new Aya::Profiling::CodeProfiler("3D CPU Total"))

    , culling(new Aya::Profiling::CodeProfiler("Culling"))
    , flip(new Aya::Profiling::CodeProfiler("Flipping Backbuffer"))
    , renderObjects(new Aya::Profiling::CodeProfiler("Render Objects"))
    , updateLighting(new Aya::Profiling::CodeProfiler("Update Lighting"))
    , adorn2D(new Aya::Profiling::CodeProfiler("Adorn 2D"))
    , adorn3D(new Aya::Profiling::CodeProfiler("Adorn 3D"))
    , visualEngineSceneUpdater(new Aya::Profiling::CodeProfiler("Visual Engine Scene Updater"))
    , finishRendering(new Aya::Profiling::CodeProfiler("Finish Rendering"))
    , renderTargetUpdate(new Aya::Profiling::CodeProfiler("RenderTarget Update"))

    , frameRateManager(new Aya::Profiling::CodeProfiler("Frame Rate Manager"))

    , textureCompositor(new Aya::Profiling::CodeProfiler("Texture Compositor"))
    , updateSceneGraph(new Aya::Profiling::CodeProfiler("Update SceneGraph"))
    , updateAllInvalidParts(new Aya::Profiling::CodeProfiler("updateAllInvalidParts"))
    , updateDynamicsAndAggregateStatics(new Aya::Profiling::CodeProfiler("updateDynamicsAndAggregateStatics"))
    , updateDynamicParts(new Aya::Profiling::CodeProfiler("updateDynamicParts"))
{
}

RenderStats::~RenderStats() {}