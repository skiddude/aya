#include "Profiler.hpp"

#include "Debug.hpp"

#if defined(AYAPROFILER) && !defined(__APPLE__)
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244 4995 4996)
#endif

#define NOMINMAX

#ifdef __ANDROID__
#include <android/log.h>

#define MICROPROFILE_PRINTF(...) __android_log_print(ANDROID_LOG_DEBUG, "MicroProfile", __VA_ARGS__)
#endif

#ifdef _WIN32
static void MicroProfileDebugPrintf(const char* format, ...)
{
    char message[256];

    va_list args;
    va_start(args, format);
    vsprintf_s(message, format, args);
    va_end(args);
}

#define MICROPROFILE_PRINTF(...) MicroProfileDebugPrintf(__VA_ARGS__)
#endif

#define MP_ASSERT(e) AYAASSERT(e)
#define MICROPROFILE_WEBSERVER 0

#ifdef AYA_PLATFORM_DURANGO
#define MICROPROFILE_WEBSERVER_PORT 4600
#define MICROPROFILE_CONTEXT_SWITCH_TRACE 0
#define getenv(name) NULL
#endif

#if defined(__APPLE__) && !defined(AYA_PLATFORM_IOS)
#include <OpenGL/gl3.h>
#define MICROPROFILE_GPU_TIMERS_GL 0
#endif

#define MICROPROFILE_IMPL
#include "microprofile/microprofile.hpp"

#define MICROPROFILE_DRAWCURSOR 1

#include "microprofile/microprofileui.hpp"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <vector>

struct InputState
{
    int mouseX, mouseY, mouseWheel;
    bool mouseButton[4];
};

Aya::Profiler::Renderer* gProfileRenderer = 0;

InputState gProfilerInputState;

void MicroProfileDrawText(int nX, int nY, uint32_t nColor, const char* pText, uint32_t nNumCharacters)
{
    gProfileRenderer->drawText(nX, nY, nColor, pText, nNumCharacters, MICROPROFILE_TEXT_WIDTH, MICROPROFILE_TEXT_HEIGHT);
}

void MicroProfileDrawBox(int nX, int nY, int nX1, int nY1, uint32_t nColor, MicroProfileBoxType type)
{
    if (type == MicroProfileBoxTypeBar)
    {
        uint32_t r = 0xff & (nColor >> 16);
        uint32_t g = 0xff & (nColor >> 8);
        uint32_t b = 0xff & nColor;

        uint32_t nMax = MicroProfileMax(MicroProfileMax(MicroProfileMax(r, g), b), 30u);
        uint32_t nMin = MicroProfileMin(MicroProfileMin(MicroProfileMin(r, g), b), 180u);

        uint32_t r0 = 0xff & ((r + nMax) / 2);
        uint32_t g0 = 0xff & ((g + nMax) / 2);
        uint32_t b0 = 0xff & ((b + nMax) / 2);

        uint32_t r1 = 0xff & ((r + nMin) / 2);
        uint32_t g1 = 0xff & ((g + nMin) / 2);
        uint32_t b1 = 0xff & ((b + nMin) / 2);

        uint32_t nColor0 = (r0 << 16) | (g0 << 8) | (b0 << 0) | (0xff000000 & nColor);
        uint32_t nColor1 = (r1 << 16) | (g1 << 8) | (b1 << 0) | (0xff000000 & nColor);

        gProfileRenderer->drawBox(nX, nY, nX1, nY1, nColor0, nColor1);
    }
    else
    {
        gProfileRenderer->drawBox(nX, nY, nX1, nY1, nColor, nColor);
    }
}

void MicroProfileDrawLine2D(uint32_t nVertices, float* pVertices, uint32_t nColor)
{
    gProfileRenderer->drawLine(nVertices, const_cast<const float*>(pVertices), nColor);
}

namespace Aya
{
namespace Profiler
{
MicroProfileTokenType getTokenType(const char* group)
{
    return strcmp(group, "GPU") == 0 ? MicroProfileTokenTypeGpu : MicroProfileTokenTypeCpu;
}

Token getToken(const char* group, const char* name, int color)
{
    return MicroProfileGetToken(group, name, color, getTokenType(group));
}

Token getLabelToken(const char* group)
{
    return MicroProfileGetLabelToken(group, getTokenType(group));
}

Token getCounterToken(const char* name)
{
    return MicroProfileGetCounterToken(name);
}

uint64_t enterRegion(Token token)
{
    return MicroProfileEnter(token);
}

void addLabel(Token token, const char* name)
{
    MicroProfileLabel(token, name);
}

void addLabelFormat(Token token, const char* name, ...)
{
    va_list args;
    va_start(args, name);
    MicroProfileLabelFormatV(token, name, args);
    va_end(args);
}

void leaveRegion(Token token, uint64_t enterTimestamp)
{
    MicroProfileLeave(token, enterTimestamp);
}

void counterAdd(Token token, long long count)
{
    MicroProfileCounterAdd(token, count);
}

void counterSet(Token token, long long count)
{
    MicroProfileCounterSet(token, count);
}

void onThreadCreate(const char* name)
{
    MicroProfileOnThreadCreate(name);
}

void onThreadExit()
{
    MicroProfileOnThreadExit();
}

void onFrame()
{
    if (g_MicroProfile.nWebServerDataSent)
    {
        // Set the defaults for web serving after the first connect
        MicroProfileSetEnableAllGroups(true);
    }

    MicroProfileFlip();
}

void gpuInit(void* context)
{
    MicroProfileGpuInit(context);
}

void gpuShutdown()
{
    MicroProfileGpuShutdown();
}

bool isCapturingMouseInput()
{
    return (MicroProfileIsDrawing() && !g_MicroProfile.nRunning);
}

bool handleMouse(unsigned int flags, int mouseX, int mouseY, int mouseWheel, int mouseButton)
{
    if (flags & Flag_MouseMove)
    {
        gProfilerInputState.mouseX = mouseX;
        gProfilerInputState.mouseY = mouseY;
    }

    if (flags & Flag_MouseWheel)
        gProfilerInputState.mouseWheel = mouseWheel;

    if (flags & Flag_MouseDown)
        gProfilerInputState.mouseButton[mouseButton] = true;

    if (flags & Flag_MouseUp)
        gProfilerInputState.mouseButton[mouseButton] = false;

    MicroProfileMousePosition(std::max(gProfilerInputState.mouseX, 0), std::max(gProfilerInputState.mouseY, 0), gProfilerInputState.mouseWheel);
    MicroProfileMouseButton(gProfilerInputState.mouseButton[0], gProfilerInputState.mouseButton[1]);

    return isCapturingMouseInput();
}

bool toggleVisible()
{
    if (MicroProfileIsDrawing())
    {
        MicroProfileSetDisplayMode(MP_DRAW_OFF);

        if (!g_MicroProfile.nRunning)
            g_MicroProfile.nToggleRunning = 1;
    }
    else
    {
        MicroProfileSetDisplayMode(MP_DRAW_FRAME);
    }

    return true;
}

bool forceOn()
{
    {
        MicroProfileSetDisplayMode(MP_DRAW_FRAME);
    }

    return true;
}

bool forceOff()
{
    {
        MicroProfileSetDisplayMode(MP_DRAW_OFF);

        if (!g_MicroProfile.nRunning)
            g_MicroProfile.nToggleRunning = 1;
    }

    return false;
}

bool togglePause()
{
    if (MicroProfileIsDrawing())
    {
        if (g_MicroProfile.nRunning && g_MicroProfile.nDisplay == MP_DRAW_FRAME)
            MicroProfileSetDisplayMode(MP_DRAW_DETAILED);
        else if (!g_MicroProfile.nRunning && g_MicroProfile.nDisplay == MP_DRAW_DETAILED)
            MicroProfileSetDisplayMode(MP_DRAW_FRAME);

        MicroProfileTogglePause();

        return true;
    }

    return false;
}

bool isVisible()
{
    return MicroProfileIsDrawing();
}

void render(Renderer* renderer, unsigned int width, unsigned int height)
{
    static bool initialized = false;

    if (!initialized)
    {
        MicroProfileInitUI();

        g_MicroProfileUI.nOpacityBackground = 0x40 << 24;
    }

    gProfileRenderer = renderer;

    MicroProfileDraw(width, height);

    gProfileRenderer = 0;

    if (!initialized)
    {
        // MicroProfileDraw loads the preset; after it has loaded it we can check if
        // we need to set up the defaults to enable all groups
        if (g_MicroProfile.nActiveGroupWanted == 0)
            MicroProfileSetEnableAllGroups(true);

        initialized = true;
    }

    gProfilerInputState.mouseWheel = 0;
}
} // namespace Profiler
} // namespace Aya
#else
namespace Aya
{
namespace Profiler
{
Token getToken(const char* group, const char* name, int color)
{
    return 0;
}

uint64_t enterRegion(Token token)
{
    return 0;
}

void addLabel(Token token, const char* name) {}

void leaveRegion(Token token, uint64_t enterTimestamp) {}

void onThreadCreate(const char* name) {}

void onThreadExit() {}

void onFrame() {}

void gpuInit(void* context) {}

void gpuShutdown() {}

bool isCapturingMouseInput()
{
    return false;
}

bool handleMouse(unsigned int flags, int mouseX, int mouseY, int mouseWheel, int mouseButton)
{
    return false;
}

bool toggleVisible()
{
    return false;
}

bool togglePause()
{
    return false;
}

bool isVisible()
{
    return false;
}

void render(Renderer* renderer, unsigned int width, unsigned int height) {}
} // namespace Profiler
} // namespace Aya
#endif
