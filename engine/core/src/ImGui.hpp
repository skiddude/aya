#pragma once

#include <imgui.h>
#include <implot.h>

#define AYAIMGUI

namespace Aya
{
namespace ImGui
{

void onFrame();

enum Flags
{
    Flag_MouseMove = 1 << 0,
    Flag_MouseWheel = 1 << 1,
    Flag_MouseDown = 1 << 2,
    Flag_MouseUp = 1 << 3,
};

void gpuInit();
void gpuShutdown();

bool isCapturingMouseInput();
bool handleMouse(unsigned int flags, int mouseX, int mouseY, int mouseWheel, int mouseButton);
bool isInitialized();

struct Renderer
{
    virtual ~Renderer() {}

    virtual void draw(ImDrawData* data) = 0;
};

void getFont(unsigned char** pixels, int* w, int* h);
void render(Renderer* renderer, unsigned int width, unsigned int height);
} // namespace ImGui
} // namespace Aya