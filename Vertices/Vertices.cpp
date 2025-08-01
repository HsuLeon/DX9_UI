#include <d3d9.h>
#pragma comment (lib, "d3d9.lib")

#define WINDOW_CLASS_NAME L"D3DWindowClass"

LPDIRECT3D9 d3d = nullptr;
LPDIRECT3DDEVICE9 d3ddev = nullptr;
HWND hWnd = nullptr;

struct CUSTOMVERTEX {
    float x, y, z, rhw;
    DWORD color;
};

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE)

int windowWidth = 800;
int windowHeight = 600;
D3DPRESENT_PARAMETERS d3dpp{}; // 全域變數

void ResetDevice() {
    if (!d3ddev) return;

    // 重新設定 present parameters
    d3dpp.BackBufferWidth = windowWidth;
    d3dpp.BackBufferHeight = windowHeight;

    HRESULT hr = d3ddev->Reset(&d3dpp);
    if (FAILED(hr)) {
        // Reset 失敗可能是裝置還在使用中，通常要處理 DEVICELOST 狀況
        if (hr == D3DERR_DEVICELOST) {
            // 等待裝置可重設
            while (d3ddev->TestCooperativeLevel() == D3DERR_DEVICELOST) {
                Sleep(100);
            }
            d3ddev->Reset(&d3dpp);
        }
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_SIZE:
        windowWidth = LOWORD(lParam);
        windowHeight = HIWORD(lParam);
        if (d3ddev) ResetDevice();
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void InitD3D(HWND hWnd) {

    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow = hWnd;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    d3dpp.BackBufferWidth = windowWidth;
    d3dpp.BackBufferHeight = windowHeight;

    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    d3d->CreateDevice(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        hWnd,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,
        &d3dpp,
        &d3ddev);
}

void RenderFrame() {
    if (!d3ddev) return;

    d3ddev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 100), 1.0f, 0);
    if (SUCCEEDED(d3ddev->BeginScene())) {
        float rectWidth = 200;
        float rectHeight = 150;

        float centerX = windowWidth / 2.0f;
        float centerY = windowHeight / 2.0f;

        float left = centerX - rectWidth / 2.0f;
        float right = centerX + rectWidth / 2.0f;
        float top = centerY - rectHeight / 2.0f;
        float bottom = centerY + rectHeight / 2.0f;

        CUSTOMVERTEX vertices[] = {
            { left,  top,    0.0f, 1.0f, D3DCOLOR_XRGB(255, 0, 0) },
            { right, top,    0.0f, 1.0f, D3DCOLOR_XRGB(0, 255, 0) },
            { left,  bottom, 0.0f, 1.0f, D3DCOLOR_XRGB(0, 0, 255) },
            { right, bottom, 0.0f, 1.0f, D3DCOLOR_XRGB(255, 255, 0) },
        };

        short indices[] = { 0, 1, 2, 2, 1, 3 };

        d3ddev->SetFVF(D3DFVF_CUSTOMVERTEX);
        d3ddev->DrawIndexedPrimitiveUP(
            D3DPT_TRIANGLELIST,
            0,
            4,
            2,
            indices,
            D3DFMT_INDEX16,
            vertices,
            sizeof(CUSTOMVERTEX)
        );

        d3ddev->EndScene();
    }

    d3ddev->Present(nullptr, nullptr, nullptr, nullptr);
}

void CleanD3D() {
    if (d3ddev) d3ddev->Release();
    if (d3d) d3d->Release();
}

// 找出第二個螢幕的 HMONITOR
HMONITOR GetSecondMonitor() {
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
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

    MSG msg{};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            RenderFrame();
        }
    }

    CleanD3D();
    UnregisterClass(WINDOW_CLASS_NAME, wc.hInstance);
    return 0;
}
