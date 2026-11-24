#include <d3d11.h>
#include "app.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

static ID3D11Device* dev;
static ID3D11DeviceContext* ctx;
static IDXGISwapChain* sc;
static ID3D11RenderTargetView* rtv;

static void MakeRtv() {
    ID3D11Texture2D* bb = nullptr;
    sc->GetBuffer(0, IID_PPV_ARGS(&bb));
    dev->CreateRenderTargetView(bb, nullptr, &rtv);
    bb->Release();
}

static void DropRtv() {
    if (rtv) { rtv->Release(); rtv = nullptr; }
}

static LRESULT CALLBACK Proc_(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (ImGui_ImplWin32_WndProcHandler(h, m, w, l)) return true;
    if (m == WM_SIZE && dev && w != SIZE_MINIMIZED) {
        DropRtv();
        sc->ResizeBuffers(0, LOWORD(l), HIWORD(l), DXGI_FORMAT_UNKNOWN, 0);
        MakeRtv();
        return 0;
    }
    if (m == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProc(h, m, w, l);
}
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
int WINAPI WinMain(HINSTANCE hi, HINSTANCE, LPSTR, int) {
    WNDCLASSEX wc{sizeof(wc), CS_CLASSDC, Proc_, 0, 0, hi, nullptr, nullptr, nullptr, nullptr, L"Fade1079's Dumper", nullptr};
    RegisterClassEx(&wc);
    HWND hw = CreateWindow(wc.lpszClassName, L"Fade1079's Dumper", WS_OVERLAPPEDWINDOW, 100, 100, 860, 600, nullptr, nullptr, hi, nullptr);

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hw;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    D3D_FEATURE_LEVEL fl[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    D3D_FEATURE_LEVEL got;
    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, fl, 2, D3D11_SDK_VERSION, &sd, &sc, &dev, &got, &ctx))) return 1;
    MakeRtv();

    ShowWindow(hw, SW_SHOWDEFAULT);
    UpdateWindow(hw);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ImGui::GetStyle().WindowRounding = 0.f;
    ImGui::GetStyle().FrameRounding = 4.f;
    ImGui_ImplWin32_Init(hw);
    ImGui_ImplDX11_Init(dev, ctx);

    App app;
    MSG msg{};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        app.Draw();
        ImGui::Render();
        const float clr[4] = {0.07f, 0.07f, 0.09f, 1.f};
        ctx->OMSetRenderTargets(1, &rtv, nullptr);
        ctx->ClearRenderTargetView(rtv, clr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        sc->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    DropRtv();
    sc->Release();
    ctx->Release();
    dev->Release();
    DestroyWindow(hw);
    return 0;
}
