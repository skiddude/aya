#include "ContextGL.hpp"

#include "HeadersGL.hpp"

#include "Debug.hpp"

#include <Windows.h>

#include <glad/gl.h>
#include <glad/wgl.h>

#include "Utility/StandardOut.hpp"


FASTFLAG(DebugGraphicsGL)

namespace Aya
{
namespace Graphics
{
class ContextGLWin32 : public ContextGL
{
public:
    ContextGLWin32(void* windowHandle)
        : hwnd(reinterpret_cast<HWND>(windowHandle))
        , hdc(GetDC(hwnd))
        , hglrc(NULL)
    {
        PIXELFORMATDESCRIPTOR pfd = {};
        pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.cAlphaBits = 8;
        pfd.cDepthBits = 24;
        pfd.cStencilBits = 8;
        pfd.iLayerType = PFD_MAIN_PLANE;

        int pf = ChoosePixelFormat(hdc, &pfd);

        if (!pf)
            throw Aya::runtime_error("Error choosing pixel format: %x", GetLastError());

        if (!SetPixelFormat(hdc, pf, &pfd))
            throw Aya::runtime_error("Error setting pixel format: %x", GetLastError());

        hglrc = wglCreateContext(hdc);

        if (!hglrc)
            throw Aya::runtime_error("Error creating context: %x", GetLastError());

        if (!wglMakeCurrent(hdc, hglrc))
            throw Aya::runtime_error("Error changing context: %x", GetLastError());

        if (!gladLoaderLoadWGL(hdc))
            throw Aya::runtime_error("Failed to initialize GLAD WGL with hdc");

        // Initialize GLAD
        if (!gladLoaderLoadGL())
            throw Aya::runtime_error("Failed to initialize GLAD");

        if (GLAD_WGL_ARB_create_context)
        {
            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(hglrc);

            const int attribs[] = {WGL_CONTEXT_MAJOR_VERSION_ARB, 3, WGL_CONTEXT_MINOR_VERSION_ARB, 2, 0, 0};

            hglrc = wglCreateContextAttribsARB(hdc, NULL, attribs);

            if (!hglrc)
                throw Aya::runtime_error("Error creating context: %x", GetLastError());

            if (!wglMakeCurrent(hdc, hglrc))
                throw Aya::runtime_error("Error changing context: %x", GetLastError());

            // Reinitialize GLAD with the new context
            if (!gladLoaderLoadGL())
                throw Aya::runtime_error("Failed to reinitialize GLAD");
        }

        if (GLAD_WGL_EXT_swap_control)
            wglSwapIntervalEXT(0);
    }

    ~ContextGLWin32()
    {
        if (wglGetCurrentContext() == hglrc)
            wglMakeCurrent(NULL, NULL);

        wglDeleteContext(hglrc);
    }

    virtual void setCurrent()
    {
        if (wglGetCurrentContext() != hglrc)
        {
            BOOL result = wglMakeCurrent(hdc, hglrc);
            AYAASSERT(result);
        }
    }

    virtual void swapBuffers()
    {
        AYAASSERT(wglGetCurrentContext() == hglrc);

        SwapBuffers(hdc);
    }

    virtual unsigned int getMainFramebufferId()
    {
        return 0;
    }

    virtual bool isMainFramebufferRetina()
    {
        return false;
    }

    virtual std::pair<unsigned int, unsigned int> updateMainFramebuffer(unsigned int width, unsigned int height)
    {
        RECT rect = {};
        GetClientRect(hwnd, &rect);

        return std::make_pair(rect.right - rect.left, rect.bottom - rect.top);
    }

    virtual void resizeWindow(int w, int h)
    {
        return;
    }

private:
    HWND hwnd;
    HDC hdc;
    HGLRC hglrc;
};

ContextGL* ContextGL::create(void* windowHandle, void* display, int w, int h)
{
    return new ContextGLWin32(windowHandle);
}

} // namespace Graphics
} // namespace Aya
