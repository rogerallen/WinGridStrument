// WinGridStrument.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "WinGridStrument.h"
#include "map"
#include <cassert>
#include "GridPointer.h"
#include <iostream>
#include <fstream>
#include <d2d1.h>
#pragma comment(lib, "d2d1")

const static int MAX_LOADSTRING = 100;

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// D2D vars -- really globals?
ID2D1Factory* pFactory;
ID2D1HwndRenderTarget* pRenderTarget;

std::map<int, GridPointer> gridPointers;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

HRESULT CreateGraphicsResources(HWND);
void DiscardGraphicsResources();
void OnResize(HWND hWnd);
void OnPaint(HWND hWnd);

void OnPointerDownHandler(HWND hWnd, const POINTER_TOUCH_INFO& pti);
void OnPointerUpdateHandler(HWND hWnd, const POINTER_TOUCH_INFO& pti);
void OnPointerUpHandler(HWND hWnd, const POINTER_TOUCH_INFO& pti);
void ScreenToClient(HWND hWnd, RECT* r);

// ======================================================================
template <class T> void SafeRelease(T** ppT) {
    if (*ppT) {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

// ======================================================================
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

#ifndef NDEBUG
    std::ofstream out("logfile.txt");
    std::streambuf* coutbuf = std::cout.rdbuf(); //save old buf
    std::cout.rdbuf(out.rdbuf()); //redirect std::cout
#endif

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_WINGRIDSTRUMENT, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINGRIDSTRUMENT));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}

// ======================================================================
//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINGRIDSTRUMENT));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_WINGRIDSTRUMENT);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

// ======================================================================
//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

// ======================================================================
//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_POINTERDOWN:
    {
        POINTER_INPUT_TYPE pointer_type;
        GetPointerType(GET_POINTERID_WPARAM(wParam), &pointer_type);
        if (pointer_type == PT_TOUCH) {
            POINTER_TOUCH_INFO pti;
            GetPointerTouchInfo(GET_POINTERID_WPARAM(wParam), &pti);
            OnPointerDownHandler(hWnd, pti);
        }
    }
    break;
    case WM_POINTERUPDATE:
    {
        POINTER_INPUT_TYPE pointer_type;
        GetPointerType(GET_POINTERID_WPARAM(wParam), &pointer_type);
        if (pointer_type == PT_TOUCH) {
            POINTER_TOUCH_INFO pti;
            GetPointerTouchInfo(GET_POINTERID_WPARAM(wParam), &pti);
            OnPointerUpdateHandler(hWnd, pti);
        }
    }
    break;
    case WM_POINTERUP:
    {
        POINTER_INPUT_TYPE pointer_type;
        GetPointerType(GET_POINTERID_WPARAM(wParam), &pointer_type);
        if (pointer_type == PT_TOUCH) {
            POINTER_TOUCH_INFO pti;
            GetPointerTouchInfo(GET_POINTERID_WPARAM(wParam), &pti);
            OnPointerUpHandler(hWnd, pti);
        }
    }
    break;
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_SIZE:
        OnResize(hWnd);
        break;
    case WM_PAINT:
        OnPaint(hWnd);
        break;
    case WM_CREATE:
        if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory))) {
            return -1;  // Fail CreateWindowEx.
        }
        break;
    case WM_DESTROY:
        DiscardGraphicsResources();
        SafeRelease(&pFactory);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// ======================================================================
// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// ======================================================================
HRESULT CreateGraphicsResources(HWND hWnd)
{
    HRESULT hr = S_OK;
    if (pRenderTarget == NULL) {
        RECT rc;
        GetClientRect(hWnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        hr = pFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(hWnd, size),
            &pRenderTarget);

    }
    return hr;
}
void DiscardGraphicsResources()
{
    SafeRelease(&pRenderTarget);
}

void OnResize(HWND hWnd) {
    if (pRenderTarget != NULL) {
        RECT rc;
        GetClientRect(hWnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);
        pRenderTarget->Resize(size);
        InvalidateRect(hWnd, NULL, FALSE);
    }
}

void OnPaint(HWND hWnd) {
    HRESULT hr = CreateGraphicsResources(hWnd);
    if (SUCCEEDED(hr))
    {
        PAINTSTRUCT ps;
        BeginPaint(hWnd, &ps);

        pRenderTarget->BeginDraw();

        pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::SkyBlue));
        for (auto p : gridPointers) {
            ID2D1SolidColorBrush* pBrush;
            float pressure = p.second.pressure() / 512.0f;
            const D2D1_COLOR_F color = D2D1::ColorF(pressure, 0.0f, pressure);
            HRESULT hr = pRenderTarget->CreateSolidColorBrush(color, &pBrush);
            if (SUCCEEDED(hr)) {
                RECT rc = p.second.rect();
                D2D1_RECT_F rcf = D2D1::RectF((float)rc.left, (float)rc.top, (float)rc.right, (float)rc.bottom);
                pRenderTarget->FillRectangle(&rcf, pBrush);
                SafeRelease(&pBrush);
            }
        }

        hr = pRenderTarget->EndDraw();
        if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET) {
            DiscardGraphicsResources();
        }
        EndPaint(hWnd, &ps);
    }
}

// ======================================================================
void OnPointerDownHandler(HWND hWnd, const POINTER_TOUCH_INFO& pti)
{
    int id = pti.pointerInfo.pointerId;
#ifndef NDEBUG
    for (auto pair : gridPointers) {
        assert(pair.first != id);
    }
#endif    
    POINT xy = pti.pointerInfo.ptPixelLocation;
    RECT r = pti.rcContact;
    ScreenToClient(hWnd, &xy);
    ScreenToClient(hWnd, &r);
    gridPointers.emplace(id, GridPointer(id, r, xy, pti.pressure));
    InvalidateRect(hWnd, NULL, FALSE);
}

// ======================================================================
void OnPointerUpdateHandler(HWND hWnd, const POINTER_TOUCH_INFO& pti)
{
    int id = pti.pointerInfo.pointerId;
#ifndef NDEBUG
    bool found = false;
    for (auto pair : gridPointers) {
        if (pair.first == id) {
            found = true;
        }
    }
    assert(found);
#endif
    auto& p = gridPointers[id];
    POINT xy = pti.pointerInfo.ptPixelLocation;
    RECT r = pti.rcContact;
    ScreenToClient(hWnd, &xy);
    ScreenToClient(hWnd, &r);
#ifndef NDEBUG
    // seems that pti.pressure is always 512 for fingers.
    //std::cout << "touch id=" << id << " pressure=" << pti.pressure << std::endl;
#endif
    p.update(r, xy, pti.pressure);
    InvalidateRect(hWnd, NULL, FALSE);
}

// ======================================================================
void OnPointerUpHandler(HWND hWnd, const POINTER_TOUCH_INFO& pti)
{
    int id = pti.pointerInfo.pointerId;
#ifndef NDEBUG
    bool found = false;
    for (auto pair : gridPointers) {
        if (pair.first == id) {
            found = true;
        }
    }
    assert(found);
#endif
    gridPointers.erase(id);
    InvalidateRect(hWnd, NULL, FALSE);
}

void ScreenToClient(HWND hWnd, RECT* r)
{
    POINT lt, rb;
    lt.x = r->left;
    lt.y = r->top;
    rb.x = r->right;
    rb.y = r->bottom;
    ScreenToClient(hWnd, &lt);
    ScreenToClient(hWnd, &rb);
    r->left = lt.x;
    r->top = lt.y;
    r->right = rb.x;
    r->bottom = rb.y;
}