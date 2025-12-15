#pragma once

#include "Base/FrameRateManager.hpp"
#include "TextureRef.hpp"
#include "Core/Resource.hpp"
#include "ScreenSpaceEffect.hpp"

namespace Aya
{
namespace Graphics
{

class Framebuffer;
class VisualEngine;
class DeviceContext;

class SSAO : public Resource
{
public:
    struct Data;

    SSAO(VisualEngine* visualEngine);
    ~SSAO();

    bool isActive() const
    {
        return !!data.get();
    }

    Data* getData() const { return data.get(); }

    void update(SSAOLevel level, unsigned int width, unsigned int height);

    void renderCompute(DeviceContext* context);
    void renderApply(DeviceContext* context);
    void renderComposit(DeviceContext* context);

    virtual void onDeviceLost();

private:
    VisualEngine* visualEngine;

    shared_ptr<Texture> noise;

    bool forceDisabled;
    SSAOLevel currentLevel;
    scoped_ptr<Data> data;

    Data* createData(unsigned int width, unsigned int height);
    //    void renderFullscreen(DeviceContext* context, const char* vsName, const char* fsName);
};

} // namespace Graphics
} // namespace Aya
