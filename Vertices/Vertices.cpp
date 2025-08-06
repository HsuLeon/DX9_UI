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
LPDIRECT3DTEXTURE9 renderTexture = NULL;
LPDIRECT3DSURFACE9 renderSurface = NULL;
LPDIRECT3DSURFACE9 pSrcBackBuffer = NULL;
LPDIRECT3DSURFACE9 pSrcZBuffer = NULL;
LPD3DXSPRITE sprite = NULL;
int tetureWidth = 512;
int tetureHeight = 512;


HWND hWnd = nullptr;
spine::SkeletonDrawable* g_pSkeletonDrawable = nullptr;

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
    case WM_KEYDOWN:
        switch (wParam) {
        case '1':
            // 你可以在這裡設定動畫或執行其他邏輯
            g_pSkeletonDrawable->getAnimationState()->setAnimation(0, "aim", true);
            break;
        case '2':
            g_pSkeletonDrawable->getAnimationState()->setAnimation(0, "jump", false);
            g_pSkeletonDrawable->getAnimationState()->addAnimation(0, "run", true, 0);
            break;
        case '3':
            g_pSkeletonDrawable->getAnimationState()->setAnimation(0, "shoot", false);
            g_pSkeletonDrawable->getAnimationState()->addAnimation(0, "run", true, 0);
            break;
        case '4':
            g_pSkeletonDrawable->getAnimationState()->setAnimation(0, "walk", true);
            break;
        case '5':
            g_pSkeletonDrawable->getAnimationState()->setAnimation(0, "portal", false);
            g_pSkeletonDrawable->getAnimationState()->addAnimation(0, "run", true, 0);
            break;
        case 37:
            g_pSkeletonDrawable->setX(g_pSkeletonDrawable->getX() - 1.0f);
            break;
        case 38:
            g_pSkeletonDrawable->setY(g_pSkeletonDrawable->getY() - 1.0f);
            break;
        case 39:
            g_pSkeletonDrawable->setX(g_pSkeletonDrawable->getX() + 1.0f);
            break;
        case 40:
            g_pSkeletonDrawable->setY(g_pSkeletonDrawable->getY() + 1.0f);
            break;
        }
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void InitD3D(HWND hWnd)
{
    // Set up the presentation parameters
    SecureZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.BackBufferCount = 1;
    d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;

    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    d3dpp.BackBufferWidth = windowWidth;
    d3dpp.BackBufferHeight = windowHeight;
    d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

    g_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    g_pD3D->CreateDevice(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        hWnd,
        D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,
        //D3DCREATE_SOFTWARE_VERTEXPROCESSING,
        &d3dpp,
        &g_pd3dDevice);

    D3DCAPS9 caps;
    g_pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);
    if (caps.MaxTextureWidth < 1024 || caps.MaxTextureHeight < 1024)
    {
        MessageBox(NULL, L"顯示卡不支援 1024x1024 的貼圖", L"錯誤", MB_OK | MB_ICONERROR);
    }
}

void CleanD3D()
{
    if (sprite) sprite->Release();
    if (renderSurface) renderSurface->Release();
    if (renderTexture) renderTexture->Release();
    if (g_pd3dDevice) g_pd3dDevice->Release();
    if (g_pD3D) g_pD3D->Release();
}

void InitRenderTarget()
{
    HRESULT hr = g_pd3dDevice->GetRenderTarget(0, &pSrcBackBuffer);
    hr = g_pd3dDevice->GetDepthStencilSurface(&pSrcZBuffer);

    hr = g_pd3dDevice->CreateTexture(tetureWidth, tetureHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &renderTexture, NULL);
    hr = renderTexture->GetSurfaceLevel(0, &renderSurface);
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
    if (!secondMon || !GetMonitorInfo(secondMon, &mi))
    {
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

    InitRenderTarget();

    HRESULT hr = D3DXCreateSprite(g_pd3dDevice, &sprite);

    bool bTest = true;
    std::string atlasPath = bTest ? "data/test/HCG-SSR-02.atlas" : "data/spineboy-pma/spineboy-pma.atlas";
    std::string jsonPath = bTest ? "data/test/HCG-SSR-02.json" : "data/spineboy-pma/spineboy-pro.json";

    spine::DX9TextureLoader textureLoader(g_pd3dDevice);
    spine::Atlas atlas(atlasPath.c_str(), &textureLoader);
    spine::AtlasAttachmentLoader attachmentLoader(&atlas);
    spine::SkeletonJson json(&attachmentLoader);
    json.setScale(0.1f);
    spine::SkeletonData* skeletonData = json.readSkeletonDataFile(jsonPath.c_str());
	skeletonData->setReferenceScale(0.1f);
    spine::SkeletonDrawable drawable(skeletonData);
    drawable.setSkin("default");
    //drawable.setSkin("03_clothes");
    drawable.getAnimationState()->getData()->setDefaultMix(0.2f);
    drawable.setX(250);
    drawable.setY(450);
    drawable.getSkeleton()->setToSetupPose();
    if (bTest)
    {
        drawable.getAnimationState()->setAnimation(0, "01_05_in", true);
        drawable.getAnimationState()->addAnimation(0, "01_05_in", true, 0);
    }
    else
    {
        drawable.getAnimationState()->setAnimation(0, "portal", true);
        drawable.getAnimationState()->addAnimation(0, "run", true, 0);
    }
    drawable.update(0, spine::Physics_Update);
	g_pSkeletonDrawable = &drawable;

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

            g_pSkeletonDrawable->update(deltaTime, spine::Physics_Update);
            if (g_pd3dDevice)
            {
                // 🔁 Render to texture
                HRESULT hr = g_pd3dDevice->SetRenderTarget(0, renderSurface);
                hr = g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0xffffffff, 1.0f, 0);
                if (SUCCEEDED(g_pd3dDevice->BeginScene()))
                {
                    // 🔷 在 render target 上畫圖形
                    g_pSkeletonDrawable->draw(g_pd3dDevice);
                    g_pd3dDevice->EndScene();
                }

                // 🔁 回到 backbuffer
                g_pd3dDevice->SetRenderTarget(0, pSrcBackBuffer);
                g_pd3dDevice->SetDepthStencilSurface(pSrcZBuffer);
                g_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 100), 1.0f, 0);
                if (SUCCEEDED(g_pd3dDevice->BeginScene()))
                {
                    // 目前視窗大小
                    float winWidth = (float)windowWidth;
                    float winHeight = (float)windowHeight;
                    // 計算寬高比例（保持不變形）
					float scale = winWidth / tetureWidth < winHeight / tetureHeight ? winWidth / tetureWidth : winHeight / tetureHeight;
                    scale = 1;
                    //scale *= 0.5f;
                    float scaleX = scale;// (winWidth - 2) / tetureWidth;// 
                    float scaleY = scale;// (winHeight - 2) / tetureHeight;// 
                    // 計算實際縮放後大小
                    float scaledWidth = tetureWidth * scaleX;
                    float scaledHeight = tetureHeight * scaleY;
                    // 計算置中偏移量
                    float offsetX = (winWidth - scaledWidth) / 2.0f;// 1.0f;// 
					float offsetY = (winHeight - scaledHeight) / 2.0f;// 1.0f;// 
                    // 建立 2D 轉換矩陣
                    D3DXMATRIX matTrans, matScale;
                    D3DXMatrixScaling(&matScale, scaleX, scaleY, 1.0f);
                    D3DXMatrixTranslation(&matTrans, offsetX, offsetY, 0.0f);
                    D3DXMATRIX matFinal = matScale * matTrans;

                    sprite->Begin(D3DXSPRITE_ALPHABLEND);
                    // 套用變換矩陣
                    sprite->SetTransform(&matFinal);
                    // 畫整張貼圖
                    sprite->Draw(renderTexture, nullptr, nullptr, nullptr, D3DCOLOR_XRGB(255, 255, 255));
                    sprite->End();

                    g_pd3dDevice->EndScene();
                }
                g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);
            }
        }
    }

    CleanD3D();
    UnregisterClass(WINDOW_CLASS_NAME, wc.hInstance);
    return 0;
}
