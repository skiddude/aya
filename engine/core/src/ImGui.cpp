#include "ImGui.hpp"
#include "DataModel/ContentProvider.hpp"

#define INPUT_CONFIG

struct InputState
{
    int mouseX, mouseY, mouseWheel;
    bool mouseButton[4];
};

InputState gImGuiInputState;
bool gImGuiContextCreated = false;
bool gImGuiFontReady = false;

#ifdef AYAIMGUI
namespace Aya
{
namespace ImGui
{

bool isInitialized()
{
    return gImGuiContextCreated && gImGuiFontReady;
}

void onFrame()
{
    if (!gImGuiContextCreated || !gImGuiFontReady)
        return;

    ::ImGui::NewFrame();
}

void gpuInit()
{
    ::IMGUI_CHECKVERSION();
    ::ImGui::CreateContext();
    ::ImPlot::CreateContext();

    // ImGui config
    ImGuiIO& imio = ::ImGui::GetIO();
    imio.DeltaTime = 1.0 / 60.0f;
    imio.DisplaySize = ImVec2(800.0f, 600.0f); // will be updated each frame
    // imio.Fonts->AddFontFromFileTTF(ContentProvider::getAssetFile("fonts/SourceSansPro-Regular.ttf").c_str(), 16.0f);
    // imio.ConfigFlags |= INPUT_CONFIG;
    // imio.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

    // ImGui style
    ImGuiStyle& style = ::ImGui::GetStyle();
    ImVec4* colors = style.Colors;

#define COL(r, g, b) ::ImGui::ColorConvertU32ToFloat4(IM_COL32(r, g, b, 255))
#define COLA(r, g, b, a) ::ImGui::ColorConvertU32ToFloat4(IM_COL32(r, g, b, a))

    // === Base ===
    colors[ImGuiCol_WindowBg] = COL(247, 247, 247);
    colors[ImGuiCol_ChildBg] = COL(255, 255, 255);
    colors[ImGuiCol_PopupBg] = COL(255, 255, 255);
    colors[ImGuiCol_Border] = COL(204, 204, 204);
    colors[ImGuiCol_FrameBg] = COL(242, 242, 242);
    colors[ImGuiCol_FrameBgHovered] = COL(230, 230, 255);
    colors[ImGuiCol_FrameBgActive] = COL(217, 217, 255);
    colors[ImGuiCol_Text] = COL(26, 26, 26);
    colors[ImGuiCol_TextDisabled] = COL(102, 102, 102);

    // === Title Bar ===
    colors[ImGuiCol_TitleBg] = COL(133, 64, 255); // #8540FF
    colors[ImGuiCol_TitleBgActive] = COL(133, 64, 255);
    colors[ImGuiCol_TitleBgCollapsed] = COL(191, 166, 255);

    // === Buttons & Headers ===
    colors[ImGuiCol_Button] = COL(235, 235, 235);
    colors[ImGuiCol_ButtonHovered] = COL(224, 224, 255);
    colors[ImGuiCol_ButtonActive] = COL(209, 209, 255);
    colors[ImGuiCol_Header] = COL(230, 217, 255);
    colors[ImGuiCol_HeaderHovered] = COL(217, 204, 255);
    colors[ImGuiCol_HeaderActive] = COL(191, 166, 255);
    colors[ImGuiCol_TextSelectedBg] = COLA(133, 64, 255, 89);

    // === Scrollbar ===
    colors[ImGuiCol_ScrollbarBg] = COL(247, 247, 247);
    colors[ImGuiCol_ScrollbarGrab] = COL(204, 204, 204);
    colors[ImGuiCol_ScrollbarGrabHovered] = COL(179, 179, 230);
    colors[ImGuiCol_ScrollbarGrabActive] = COL(153, 153, 217);

    style.WindowRounding = 6.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 2.0f;
    style.PopupRounding = 2.0f;
    style.GrabRounding = 1.0f;
    style.TabRounding = 2.0f;

#undef COL
#undef COLA

    gImGuiContextCreated = true;
}

void gpuShutdown()
{
    ::ImPlot::DestroyContext();
    ::ImGui::DestroyContext();
    gImGuiContextCreated = false;
    gImGuiFontReady = false;
}

bool isCapturingMouseInput()
{
    if (!gImGuiContextCreated)
        return false;

    return ::ImGui::GetIO().WantCaptureMouse;
}

bool handleMouse(unsigned int flags, int mouseX, int mouseY, int mouseWheel, int mouseButton)
{
    if (!gImGuiContextCreated)
        return false;

    if (flags & Flag_MouseMove)
    {
        gImGuiInputState.mouseX = mouseX;
        gImGuiInputState.mouseY = mouseY;
    }

    if (flags & Flag_MouseWheel)
        gImGuiInputState.mouseWheel = mouseWheel;

    if (flags & Flag_MouseDown)
        gImGuiInputState.mouseButton[mouseButton] = true;

    if (flags & Flag_MouseUp)
        gImGuiInputState.mouseButton[mouseButton] = false;

    ImGuiIO& imio = ::ImGui::GetIO();
    imio.MousePos = ImVec2(std::max(gImGuiInputState.mouseX, 0), std::max(gImGuiInputState.mouseY, 0));
    imio.MouseWheel = gImGuiInputState.mouseWheel;
    imio.MouseDown[0] = gImGuiInputState.mouseButton[0];
    imio.MouseDown[1] = gImGuiInputState.mouseButton[1];
    imio.MouseDown[2] = gImGuiInputState.mouseButton[2];

    return isCapturingMouseInput();
}

void getFont(unsigned char** pixels, int* w, int* h)
{
    ::ImGui::GetIO().Fonts->GetTexDataAsRGBA32(pixels, w, h);
    // The font texture data has been produced; the renderer should upload it and then
    // set the GPU texture id into ImGui (or we can treat this flag as "font ready").
    gImGuiFontReady = true;
}

void render(Renderer* renderer, unsigned int width, unsigned int height)
{
    // Need a valid context to call Render, and also ensure font/texture has been produced/uploaded.
    if (!gImGuiContextCreated)
        return;
    if (!gImGuiFontReady)
        return;

    ImGuiIO& imio = ::ImGui::GetIO();
    imio.DisplaySize = ImVec2((float)width, (float)height);

    ::ImGui::Render();

    renderer->draw(::ImGui::GetDrawData());

    gImGuiInputState.mouseWheel = 0;
}
} // namespace ImGui
} // namespace Aya
#else
namespace Aya
{
namespace ImGui
{

void onFrame() {}

void gpuInit() {}

void gpuShutdown() {}

bool isCapturingMouseInput()
{
    return false;
}

bool handleMouse(unsigned int flags, int mouseX, int mouseY, int mouseWheel, int mouseButton)
{
    return false;
}

void render(Renderer* renderer, unsigned int width, unsigned int height) {}
} // namespace ImGui
} // namespace Aya
#endif
