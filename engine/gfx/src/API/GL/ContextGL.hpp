#pragma once

#include <utility>

namespace Aya
{
namespace Graphics
{

class ContextGL
{
public:
    static ContextGL* create(void* windowHandle, void* display, int w, int h);

    virtual ~ContextGL() {}

    virtual void setCurrent() = 0;
    virtual void swapBuffers() = 0;

    virtual unsigned int getMainFramebufferId() = 0;
    virtual bool isMainFramebufferRetina() = 0;

    virtual std::pair<unsigned int, unsigned int> updateMainFramebuffer(unsigned int width, unsigned int height) = 0;
    virtual void resizeWindow(int width, int height) = 0;
};

} // namespace Graphics
} // namespace Aya
