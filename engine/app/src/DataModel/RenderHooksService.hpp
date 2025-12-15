#pragma once
#include "Tree/Instance.hpp"
#include "Tree/Service.hpp"
#include "intrusive_ptr_target.hpp"

namespace Aya
{

namespace Profiling
{
class BucketProfile;
}

struct IMainWndHooks
{
    virtual void resizeWindow(int cx, int cy) = 0;
};

struct RenderMetrics
{
    double presentTime, renderAve, deltaAve, GPUDelay;
    double renderConfidenceMin, renderConfidenceMax;
    double renderStd;
};

struct IRenderHooks
{
    virtual void captureMetrics(RenderMetrics& metrics) = 0;
    virtual void enableAdorns(bool enable) = 0;
    virtual void printScene() = 0;
};

extern const char* const sRenderHooksService;
class RenderHooksService
    : public DescribedNonCreatable<RenderHooksService, Instance, sRenderHooksService>
    , public Service
{
private:
    typedef DescribedNonCreatable<RenderHooksService, Instance, sRenderHooksService> Super;

    IMainWndHooks* wndHooks;
    IRenderHooks* renderHooks;

    RenderMetrics metrics;

public:
    RenderHooksService();

    void setMainWndHooks(IMainWndHooks* wndHooks)
    {
        this->wndHooks = wndHooks;
    }
    void setRenderHooks(IRenderHooks* renderHooks)
    {
        this->renderHooks = renderHooks;
    }

    double getPresentTime()
    {
        return metrics.presentTime;
    };
    double getGPUDelay()
    {
        return metrics.GPUDelay;
    };
    double getRenderAve()
    {
        return metrics.renderAve;
    };
    double getRenderConfMin()
    {
        return metrics.renderConfidenceMin;
    };
    double getRenderConfMax()
    {
        return metrics.renderConfidenceMax;
    };
    double getRenderStd()
    {
        return metrics.renderStd;
    };
    double getDeltaAve()
    {
        return metrics.deltaAve;
    };

    void enableAdorns(bool enabled);
    void printScene();

    Aya::signal<void()> reloadShadersSignal;
    void reloadShaders()
    {
        reloadShadersSignal();
    }

    Aya::signal<void()> resetTypesetterSignal;
    void resetTypesetter()
    {
        resetTypesetterSignal();
    }

    Aya::signal<void()> flushSignal;
    void flush()
    {
        flushSignal();
    }

    Aya::signal<void()> resetShadowMapSignal;
    void resetShadowMap()
    {
        resetShadowMapSignal();
    }

    Aya::signal<void(int)> enableQueueSignal;
    void enableQueue(int qID)
    {
        enableQueueSignal(qID);
    }

    Aya::signal<void(int)> disableQueueSignal;
    void disableQueue(int qID)
    {
        disableQueueSignal(qID);
    }

    void captureMetrics();

    void resizeWindow(int cx, int cy);
};

} // namespace Aya