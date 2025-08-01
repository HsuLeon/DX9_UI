#include <d3d9.h>
#include <d3dx9.h>
#pragma warning( disable : 4996 ) // disable deprecated warning 
#include <strsafe.h>
#pragma warning( default : 4996 )

#pragma comment (lib, "d3d9.lib")

#define WINDOW_CLASS_NAME L"D3DWindowClass"

#include "spine-dx9-cpp.h"


LPDIRECT3D9 g_pD3D = nullptr;
LPDIRECT3DDEVICE9 g_pd3dDevice = nullptr;
LPDIRECT3DTEXTURE9 g_pTexture = nullptr;
HWND hWnd = nullptr;

int windowWidth = 800;
int windowHeight = 600;
D3DPRESENT_PARAMETERS d3dpp{}; // 全域變數

void ResetDevice()
{
    if (!g_pd3dDevice) return;

    // 重新設定 present parameters
    d3dpp.BackBufferWidth = windowWidth;
    d3dpp.BackBufferHeight = windowHeight;

    HRESULT hr = g_pd3dDevice->Reset(&d3dpp);
    if (FAILED(hr)) {
        // Reset 失敗可能是裝置還在使用中，通常要處理 DEVICELOST 狀況
        if (hr == D3DERR_DEVICELOST) {
            // 等待裝置可重設
            while (g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICELOST) {
                Sleep(100);
            }
            g_pd3dDevice->Reset(&d3dpp);
        }
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_SIZE:
        windowWidth = LOWORD(lParam);
        windowHeight = HIWORD(lParam);
        if (g_pd3dDevice) ResetDevice();
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void InitD3D(HWND hWnd)
{
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow = hWnd;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    d3dpp.BackBufferWidth = windowWidth;
    d3dpp.BackBufferHeight = windowHeight;

    g_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    g_pD3D->CreateDevice(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        hWnd,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,
        &d3dpp,
        &g_pd3dDevice);

    // Use D3DX to create a texture from a file based image
    if (FAILED(D3DXCreateTextureFromFile(g_pd3dDevice, L"data\\banana.bmp", &g_pTexture)))
    {
        MessageBox(NULL, L"Could not find banana.bmp", L"Textures.exe", MB_OK);
    }
}

void RenderFrame(spine::SkeletonDrawable* pDrawable)
{
    if (!g_pd3dDevice) return;

    g_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 100), 1.0f, 0);
    if (SUCCEEDED(g_pd3dDevice->BeginScene()))
    {
        //g_pd3dDevice->SetTexture(0, g_pTexture);
        //g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
        //g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        //g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        //g_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

        //float rectWidth = 200;
        //float rectHeight = 150;

        //float centerX = windowWidth / 2.0f;
        //float centerY = windowHeight / 2.0f;

        //float left = centerX - rectWidth / 2.0f;
        //float right = centerX + rectWidth / 2.0f;
        //float top = centerY - rectHeight / 2.0f;
        //float bottom = centerY + rectHeight / 2.0f;

        //CUSTOMVERTEX vertices[] = {
        //    { left,  top,    0.0f, 1.0f, D3DCOLOR_XRGB(255, 0, 0), 0.0f, 0.0f },
        //    { right, top,    0.0f, 1.0f, D3DCOLOR_XRGB(0, 255, 0), 1.0f, 0.0f },
        //    { left,  bottom, 0.0f, 1.0f, D3DCOLOR_XRGB(0, 0, 255), 0.0f, 1.0f },
        //    { right, bottom, 0.0f, 1.0f, D3DCOLOR_XRGB(255, 255, 0), 1.0f, 1.0f },
        //};

        //short indices[] = { 0, 1, 2, 2, 1, 3 };

        //g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
        //g_pd3dDevice->DrawIndexedPrimitiveUP(
        //    D3DPT_TRIANGLELIST,
        //    0,
        //    4,
        //    2,
        //    indices,
        //    D3DFMT_INDEX16,
        //    vertices,
        //    sizeof(CUSTOMVERTEX)
        //);

        pDrawable->draw(g_pd3dDevice);

        g_pd3dDevice->EndScene();
    }

    g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);
}

void CleanD3D()
{
    if (g_pTexture) g_pTexture->Release();
    if (g_pd3dDevice) g_pd3dDevice->Release();
    if (g_pD3D) g_pD3D->Release();
}

// 找出第二個螢幕的 HMONITOR
HMONITOR GetSecondMonitor()
{
    int monitorIndex = 0;
    HMONITOR secondMonitor = nullptr;
    EnumDisplayMonitors(nullptr, nullptr,
        [](HMONITOR hMon, HDC, LPRECT, LPARAM lParam) -> BOOL {
            int* index = reinterpret_cast<int*>(lParam);
            if (*index == 1) { // 第二個螢幕（索引從0開始）
                *(HMONITOR*)lParam = hMon;
                return FALSE; // 找到就停止列舉
            }
            (*index)++;
            return TRUE;
        }, (LPARAM)&secondMonitor);
    return secondMonitor;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    WNDCLASSEX wc{ sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L,
        GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr,
        WINDOW_CLASS_NAME, nullptr };
    RegisterClassEx(&wc);

    // 找第二個螢幕
    HMONITOR secondMon = GetSecondMonitor();
    MONITORINFO mi = { sizeof(mi) };
    if (!secondMon || !GetMonitorInfo(secondMon, &mi)) {
        MessageBox(nullptr, L"找不到第二個螢幕，將使用主螢幕。", L"警告", MB_ICONWARNING);
        secondMon = MonitorFromWindow(nullptr, MONITOR_DEFAULTTOPRIMARY);
        GetMonitorInfo(secondMon, &mi);
    }

    // 取得螢幕解析度
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // 計算讓視窗出現在螢幕正中央的位置
    int windowX = (screenWidth - windowWidth) / 2;
    int windowY = (screenHeight - windowHeight) / 2;

    int centerX = mi.rcMonitor.left + (mi.rcMonitor.right - mi.rcMonitor.left - windowWidth) / 2;
    int centerY = mi.rcMonitor.top + (mi.rcMonitor.bottom - mi.rcMonitor.top - windowHeight) / 2;

    hWnd = CreateWindowW(WINDOW_CLASS_NAME, L"DirectX9 on 2nd Monitor",
        WS_OVERLAPPEDWINDOW, centerX, centerY, windowWidth, windowHeight,
        nullptr, nullptr, wc.hInstance, nullptr);

    InitD3D(hWnd);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    spine::DX9TextureLoader textureLoader(g_pd3dDevice);
    spine::Atlas atlas("data/spineboy-pma/spineboy-pma.atlas", &textureLoader);
    spine::AtlasAttachmentLoader attachmentLoader(&atlas);
    spine::SkeletonJson json(&attachmentLoader);
    json.setScale(0.5f);
    spine::SkeletonData* skeletonData = json.readSkeletonDataFile("data/spineboy-pma/spineboy-pro.json");
    spine::SkeletonDrawable drawable(skeletonData);
    drawable.usePremultipliedAlpha = true;
    drawable.animationState->getData()->setDefaultMix(0.2f);
    drawable.skeleton->setPosition(400, 500);
    drawable.skeleton->setToSetupPose();
    drawable.animationState->setAnimation(0, "portal", true);
    drawable.animationState->addAnimation(0, "run", true, 0);
    drawable.update(0, spine::Physics_Update);

    DWORD lastFrameTime = ::timeGetTime();
    MSG msg{};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {

            DWORD nowTime = ::timeGetTime();
            double deltaTime = (nowTime - lastFrameTime) / 1000.0f;
            lastFrameTime = nowTime;

            drawable.update(deltaTime, spine::Physics_Update);

            RenderFrame(&drawable);
        }
    }

    CleanD3D();
    UnregisterClass(WINDOW_CLASS_NAME, wc.hInstance);
    return 0;
}
