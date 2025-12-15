#include "ContextGL.hpp"
#include "HeadersGL.hpp"
#include "Debug.hpp"

#ifdef __APPLE__
#import <Cocoa/Cocoa.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl3.h>
#endif

LOGGROUP(Graphics)

namespace Aya
{
namespace Graphics
{

#ifdef __APPLE__
class ContextGLMacOS : public ContextGL
{
public:
    ContextGLMacOS(void* windowHandle)
    {
        // Handle both NSWindow and NSView (Qt widgets)
        NSView* view = (__bridge NSView*)windowHandle;
        if (!view) {
            printf("ERROR: Invalid window/view handle\n");
            return;
        }
        
        // Determine if it's a window or view
        NSWindow* window = nil;
        NSView* targetView = view;
        
        if ([view isKindOfClass:[NSWindow class]]) {
            window = (NSWindow*)view;
            targetView = [window contentView];
            nsWindow = window;
        } else {
            // If it's a view, try to get its window
            window = [view window];
            targetView = view;
            nsWindow = window;
        }
        
        if (!targetView) {
            printf("ERROR: Could not get target view\n");
            return;
        }
        
        printf("Creating native macOS OpenGL context...\n");
        
        // Create OpenGL pixel format
        NSOpenGLPixelFormatAttribute attrs[] = {
            NSOpenGLPFADoubleBuffer,
            NSOpenGLPFADepthSize, 24,
            NSOpenGLPFAStencilSize, 8,
            NSOpenGLPFAColorSize, 24,
            NSOpenGLPFAAlphaSize, 8,
            NSOpenGLPFAAccelerated,
            NSOpenGLPFANoRecovery,
            NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
            0
        };
        
        pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
        if (!pixelFormat) {
            printf("Failed to create OpenGL pixel format\n");
            return;
        }
        
        // Create OpenGL context
        glContext = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];
        if (!glContext) {
            printf("Failed to create OpenGL context\n");
            return;
        }
        
        // Set the target view
        [glContext setView:targetView];
        
        // Make context current
        [glContext makeCurrentContext];
        
        printf("Context created and made current successfully\n");
        
        // Test OpenGL
        const GLubyte* version = glGetString(GL_VERSION);
        if (version) {
            printf("OpenGL Version: %s\n", version);
        }
        
        const GLubyte* vendor = glGetString(GL_VENDOR);
        if (vendor) {
            printf("OpenGL Vendor: %s\n", vendor);
        }
        
        const GLubyte* renderer = glGetString(GL_RENDERER);
        if (renderer) {
            printf("OpenGL Renderer: %s\n", renderer);
        }
    }
    
    ~ContextGLMacOS()
    {
        if (glContext) {
            [NSOpenGLContext clearCurrentContext];
        }
    }
    
    virtual void setCurrent() override
    {
        if (glContext) {
            [glContext makeCurrentContext];
        }
    }
    
    virtual void swapBuffers() override
    {
        if (glContext) {
            [glContext flushBuffer];
        }
    }
    
    virtual unsigned int getMainFramebufferId() override
    {
        return 0; // Default framebuffer on macOS
    }
    
    virtual bool isMainFramebufferRetina() override
    {
        if (!nsWindow) return false;
        
        NSScreen* screen = [nsWindow screen];
        if (!screen) return false;
        
        return [screen backingScaleFactor] > 1.0;
    }
    
    virtual std::pair<unsigned int, unsigned int> updateMainFramebuffer(unsigned int width, unsigned int height) override
    {
        if (!nsWindow) {
            return std::make_pair(width, height);
        }
        
        NSView* contentView = [nsWindow contentView];
        if (!contentView) {
            return std::make_pair(width, height);
        }
        
        NSRect contentRect = [contentView bounds];
        NSRect backingRect = [contentView convertRectToBacking:contentRect];
        
        return std::make_pair((unsigned int)backingRect.size.width, 
                             (unsigned int)backingRect.size.height);
    }

    virtual void resizeWindow(int w, int h) override
    {
        return;
    }
    
private:
    NSWindow* nsWindow;
    NSOpenGLContext* glContext;
    NSOpenGLPixelFormat* pixelFormat;
};

#endif

ContextGL* ContextGL::create(void* windowHandle, void* display, int w, int h)
{
    return new ContextGLMacOS(windowHandle);
}

} // namespace Graphics
} // namespace Aya