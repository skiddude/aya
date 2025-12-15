#include "Base/ViewBase.hpp"
#include "time.hpp"

#include "Debug.hpp"

#include "Utility/MachineIdUploader.hpp"
#include "boost/functional/hash.hpp"

namespace Aya
{

extern void RenderView_InitModule();
extern void RenderView_ShutdownModule();

static IViewBaseFactory** getFactory(CRenderSettings::GraphicsMode mode)
{
    static IViewBaseFactory* s_rgFactories[6] = {0, 0, 0, 0, 0, 0};

    if ((static_cast<size_t>(mode)) < ARRAYSIZE(s_rgFactories))
    {
        return s_rgFactories + static_cast<size_t>(mode);
    }
    else
    {
        return 0;
    }
}

ViewBase* ViewBase::CreateView(CRenderSettings::GraphicsMode mode, OSContext* context, CRenderSettings* renderSettings)
{
    IViewBaseFactory** ppfactory = getFactory(mode);

    // did you call Aya::ViewBase::InitPluginModules?
    AYAASSERT(ppfactory && *ppfactory);
    if (ppfactory && *ppfactory)
    {
        return (*ppfactory)->Create(mode, context, renderSettings);
    }
    else
    {
        return 0;
    }
}

void ViewBase::RegisterFactory(CRenderSettings::GraphicsMode mode, IViewBaseFactory* factory)
{
    IViewBaseFactory** ppfactory = getFactory(mode);

    AYAASSERT(ppfactory);
    if (ppfactory)
    {
        *ppfactory = factory;
    }
}

void ViewBase::render(IMetric* metric, double timeRenderJob)
{
    if (timeRenderJob == 0.0)
        timeRenderJob = Time::nowFastSec();
    renderPrepare(metric);
    renderPerform(timeRenderJob);
}

void ViewBase::InitPluginModules()
{
    RenderView_InitModule();
}

void ViewBase::ShutdownPluginModules()
{
    RenderView_ShutdownModule();
}

std::pair<unsigned, unsigned> ViewBase::setFrameDataCallback(const boost::function<void(void*)>& callback)
{
    return std::make_pair(0, 0);
}

} // namespace Aya
