// ======================================================================
// WinGridStrument - a Windows touchscreen musical instrument
// Copyright(C) 2020 Roger Allen
// 
// This program is free software : you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
// ======================================================================
// WinGridStrument.cpp : Defines the entry point for the application.

// cannot get rid of this as WinGridStrument.rc uses it
#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// resources
#include "resource.h"

#include "GridStrument.h"
#include "GridUtils.h"

#include <fluidsynth.h>

#include <map>
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <d2d1.h>
#pragma comment(lib, "d2d1")
#include <mmsystem.h>  // multimedia functions (such as MIDI) for Windows
#pragma comment(lib, "winmm")
#include <dwrite.h>
#pragma comment(lib, "Dwrite")

const static int MAX_LOADSTRING = 100;
enum class Pref {
    MIDI_DEVICE_INDEX, GUITAR_MODE, PITCH_BEND_RANGE, PITCH_BEND_MASK,
    MODULATION_CONTROLLER, MIDI_CHANNEL_MIN, MIDI_CHANNEL_MAX,
    GRID_SIZE, CHANNEL_PER_ROW_MODE, COLOR_THEME, HEX_GRID_MODE
};

// Global Variables:
HINSTANCE g_instance;

// D2D vars -- really globals?
ID2D1Factory* g_d2dFactory;
ID2D1HwndRenderTarget* g_d2dRenderTarget;

// DirectWrite vars
IDWriteFactory* g_dwriteFactory;
IDWriteTextFormat* g_textFormat;

// MIDI vars
HMIDIOUT g_midiDevice;
int g_midiDeviceIndex;
std::vector<std::wstring> g_midiDeviceNames;

// Instrument Class Vars
GridStrument* g_gridStrument;

// FIXME - must be a better way to do this
bool g_dirty_main_window = false;

// FIXME - DPI Awareness...do we need to be aware?
// https://docs.microsoft.com/en-us/windows/win32/api/windef/ne-windef-dpi_awareness
// older deprecated (with no notice!)
// https://docs.microsoft.com/en-us/windows/win32/direct2d/how-to--size-a-window-properly-for-high-dpi-displays

// Forward declarations of functions included in this code module:
ATOM             MyRegisterClass(HINSTANCE, WCHAR*);
void             InitInstance(HINSTANCE, int, WCHAR*, WCHAR*);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK AboutCallback(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK PrefsCallback(HWND, UINT, WPARAM, LPARAM);

void InitPrefsDialog(const HWND& hDlg);
void OkUpdatePrefsDialog(const HWND& hDlg);

HRESULT CreateGraphicsResources(HWND);
void DiscardGraphicsResources();
void OnResize(HWND hWnd);
void OnPaint(HWND hWnd);

void OnPointerDownHandler(HWND hWnd, const POINTER_TOUCH_INFO& pti);
void OnPointerUpdateHandler(HWND hWnd, const POINTER_TOUCH_INFO& pti);
//void OnPointerUpdateHandler(HWND hWnd, const POINTER_PEN_INFO& ppi);
void OnPointerUpHandler(HWND hWnd, const POINTER_TOUCH_INFO& pti);

void ScreenToClient(HWND hWnd, RECT* r);

void QueryMidiDevices();
MMRESULT StartMidi();
void StopMidi();

int PrefGetDefault(Pref key);
std::wstring PrefGetLabel(Pref key);
int PrefGetInt(Pref key);
void PrefSetInt(Pref key, int value);

void AlertExit(HWND hWnd, LPCTSTR text);

// ======================================================================
// main windows entry function
// construct our program from here
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // setup wcout to save output to a logfile.txt
    std::wofstream logstream("logfile.txt");
    //std::wstreambuf* old_wcout = std::wcout.rdbuf(); // save old buf
    std::wcout.rdbuf(logstream.rdbuf());                 // redirect std::wcout

    g_midiDeviceIndex = PrefGetInt(Pref::MIDI_DEVICE_INDEX);
    MMRESULT rc = StartMidi();
    if (rc != MMSYSERR_NOERROR) {
        AlertExit(NULL, L"Error opening MIDI Output.");
    }

    g_gridStrument = new GridStrument(g_midiDevice);
    g_gridStrument->prefGuitarMode(PrefGetInt(Pref::GUITAR_MODE));
    g_gridStrument->prefPitchBendRange(PrefGetInt(Pref::PITCH_BEND_RANGE));
    g_gridStrument->prefPitchBendMask(PrefGetInt(Pref::PITCH_BEND_MASK));
    g_gridStrument->prefModulationController(PrefGetInt(Pref::MODULATION_CONTROLLER));
    g_gridStrument->prefMidiChannelRange(PrefGetInt(Pref::MIDI_CHANNEL_MIN), PrefGetInt(Pref::MIDI_CHANNEL_MAX));
    g_gridStrument->prefGridSize(PrefGetInt(Pref::GRID_SIZE));
    g_gridStrument->prefChannelPerRowMode(PrefGetInt(Pref::CHANNEL_PER_ROW_MODE));
    g_gridStrument->prefColorTheme(static_cast<Theme>(PrefGetInt(Pref::COLOR_THEME)));
    g_gridStrument->prefHexGridMode(PrefGetInt(Pref::HEX_GRID_MODE));


    WCHAR title[MAX_LOADSTRING];       // The title bar textfP
    WCHAR windowClass[MAX_LOADSTRING]; // the main window class name
    LoadStringW(hInstance, IDS_APP_TITLE, title, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_WINGRIDSTRUMENT, windowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance, windowClass);

    // Perform application initialization:
    InitInstance(hInstance, nCmdShow, windowClass, title);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINGRIDSTRUMENT));

    // Main message loop:
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (g_dirty_main_window) {  // FIXME - this doesn't always work
            InvalidateRect(msg.hwnd, NULL, FALSE);
            g_dirty_main_window = false;
        }
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    StopMidi();

    return (int)msg.wParam;
}

// ======================================================================
// Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance, WCHAR* windowClass)
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
    wcex.lpszClassName = windowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

// ======================================================================
// Saves instance handle and creates main window
// save the instance handle in a global variable and create and display 
// the main program window.
//
void InitInstance(HINSTANCE hInstance, int nCmdShow, WCHAR* windowClass, WCHAR* title)
{
    g_instance = hInstance; // Store instance handle in our global variable

    HWND hWnd = CreateWindowW(windowClass, title, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (hWnd != NULL) {
        ShowWindow(hWnd, nCmdShow);
        ShowWindow(hWnd, SW_SHOWMAXIMIZED); // maximize window on startup
        UpdateWindow(hWnd);
    }
    else {
        AlertExit(NULL, L"CreateWindow Failed.");
    }

}

// ======================================================================
// Processes messages/events for the main window.
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_POINTERDOWN: {
        POINTER_INPUT_TYPE pointer_type;
        GetPointerType(GET_POINTERID_WPARAM(wParam), &pointer_type);
        if (pointer_type == PT_TOUCH) {
            POINTER_TOUCH_INFO pti;
            GetPointerTouchInfo(GET_POINTERID_WPARAM(wParam), &pti);
            OnPointerDownHandler(hWnd, pti);
        } // we do not get (pointer_type == PT_PEN) 
        break;
    }
    case WM_POINTERUPDATE: {
        POINTER_INPUT_TYPE pointer_type;
        GetPointerType(GET_POINTERID_WPARAM(wParam), &pointer_type);
        if (pointer_type == PT_TOUCH) {
            POINTER_TOUCH_INFO pti;
            GetPointerTouchInfo(GET_POINTERID_WPARAM(wParam), &pti);
            OnPointerUpdateHandler(hWnd, pti);
        }
#if 0
        else if (pointer_type == PT_PEN) {
            // all the events for PEN go through here
            POINTER_PEN_INFO ppi;
            GetPointerPenInfo(GET_POINTERID_WPARAM(wParam), &ppi);
            OnPointerUpdateHandler(hWnd, ppi);
        }
#endif
        break;
    }
    case WM_POINTERUP: {
        POINTER_INPUT_TYPE pointer_type;
        GetPointerType(GET_POINTERID_WPARAM(wParam), &pointer_type);
        if (pointer_type == PT_TOUCH) {
            POINTER_TOUCH_INFO pti;
            GetPointerTouchInfo(GET_POINTERID_WPARAM(wParam), &pti);
            OnPointerUpHandler(hWnd, pti);
        } // don't get these (pointer_type == PT_PEN)
        break;
    }
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        // Parse the menu selections:
        switch (wmId) {
        case IDM_ABOUT:
            DialogBox(g_instance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, AboutCallback);
            break;
        case IDM_PREFS:
            DialogBox(g_instance, MAKEINTRESOURCE(IDD_PREFS_DIALOG), hWnd, PrefsCallback);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    }
    case WM_SIZE:
        OnResize(hWnd);
        break;
    case WM_PAINT:
        OnPaint(hWnd);
        break;
    case WM_CREATE:
        if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_d2dFactory))) {
            return -1;  // Fail CreateWindowEx.
        }
        if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(g_dwriteFactory),
            reinterpret_cast<IUnknown**>(&g_dwriteFactory)))) {
            return -1;  // Fail CreateWindowEx.
        }
        break;
    case WM_DESTROY:
        DiscardGraphicsResources();
        SafeRelease(&g_d2dFactory);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// ======================================================================
// Message handler for about box.
INT_PTR CALLBACK AboutCallback(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// ======================================================================
// Message handler for preferences dialog.
INT_PTR CALLBACK PrefsCallback(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {
    case WM_INITDIALOG: {
        InitPrefsDialog(hDlg);
        return (INT_PTR)TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            if (LOWORD(wParam) == IDOK) {
                OkUpdatePrefsDialog(hDlg);
            }
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// ======================================================================
// Initialize the dialog from actual values in pref variables
//
void InitPrefsDialog(const HWND& hDlg)
{
    HWND midiDeviceComboBox = GetDlgItem(hDlg, IDC_MIDI_DEV_COMBO);
    for (auto s : g_midiDeviceNames) {
        SendMessage(midiDeviceComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)s.c_str());
    }
    SendMessage(midiDeviceComboBox, CB_SETCURSEL, (WPARAM)g_midiDeviceIndex, (LPARAM)0);

    HWND colorThemeComboBox = GetDlgItem(hDlg, IDC_COLOR_THEME_COMBO);
    for (auto s : ThemeNames) {
        SendMessage(colorThemeComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)s.c_str());
    }
    SendMessage(colorThemeComboBox, CB_SETCURSEL, (WPARAM)static_cast<int>(g_gridStrument->prefColorTheme()), (LPARAM)0);

    CheckDlgButton(hDlg, IDC_GUITAR_MODE, g_gridStrument->prefGuitarMode());

    int value = g_gridStrument->prefPitchBendRange();
    std::wstring tmp_str = std::to_wstring(value);
    SetDlgItemText(hDlg, IDC_PITCH_BEND_RANGE, tmp_str.c_str());

    value = g_gridStrument->prefPitchBendMask();
    std::wstringstream tmp_ss;
    tmp_ss << L"0x" << std::hex << value << std::dec;
    SetDlgItemText(hDlg, IDC_PITCH_BEND_MASK, tmp_ss.str().c_str());

    value = g_gridStrument->prefModulationController();
    tmp_str = std::to_wstring(value);
    SetDlgItemText(hDlg, IDC_MODULATION_CONTROLLER, tmp_str.c_str());

    value = g_gridStrument->prefMidiChannelMin();
    tmp_str = std::to_wstring(value);
    SetDlgItemText(hDlg, IDC_MIDI_CHANNEL_MIN, tmp_str.c_str());

    value = g_gridStrument->prefMidiChannelMax();
    tmp_str = std::to_wstring(value);
    SetDlgItemText(hDlg, IDC_MIDI_CHANNEL_MAX, tmp_str.c_str());

    value = g_gridStrument->prefGridSize();
    tmp_str = std::to_wstring(value);
    SetDlgItemText(hDlg, IDC_GRID_SIZE, tmp_str.c_str());

    CheckDlgButton(hDlg, IDC_CHANNEL_PER_ROW_MODE, g_gridStrument->prefChannelPerRowMode());

    CheckDlgButton(hDlg, IDC_HEX_GRID_MODE, g_gridStrument->prefHexGridMode());

}

// ======================================================================
// after user says "Ok", take the values from the prefs dialog and 
// store them in actual pref variables
//
void OkUpdatePrefsDialog(const HWND& hDlg)
{
    bool guitar_mode = IsDlgButtonChecked(hDlg, IDC_GUITAR_MODE);
    g_gridStrument->prefGuitarMode(guitar_mode);
    PrefSetInt(Pref::GUITAR_MODE, guitar_mode);

    wchar_t pitch_range_text[32];
    GetDlgItemText(hDlg, IDC_PITCH_BEND_RANGE, pitch_range_text, 32);
    wchar_t* end_ptr;
    int value = static_cast<int>(wcstol(pitch_range_text, &end_ptr, 10));
    g_gridStrument->prefPitchBendRange(value);
    PrefSetInt(Pref::PITCH_BEND_RANGE, value);

    wchar_t pitch_mask_text[32];
    GetDlgItemText(hDlg, IDC_PITCH_BEND_MASK, pitch_mask_text, 32);
    value = static_cast<int>(wcstol(pitch_mask_text, &end_ptr, 16));
    g_gridStrument->prefPitchBendMask(value);
    PrefSetInt(Pref::PITCH_BEND_MASK, value);

    wchar_t controller_text[32];
    GetDlgItemText(hDlg, IDC_MODULATION_CONTROLLER, controller_text, 32);
    value = static_cast<int>(wcstol(controller_text, &end_ptr, 10));
    g_gridStrument->prefModulationController(value);
    PrefSetInt(Pref::MODULATION_CONTROLLER, value);

    wchar_t midi_channel_min_text[32];
    GetDlgItemText(hDlg, IDC_MIDI_CHANNEL_MIN, midi_channel_min_text, 32);
    value = static_cast<int>(wcstol(midi_channel_min_text, &end_ptr, 10));
    PrefSetInt(Pref::MIDI_CHANNEL_MIN, value);

    wchar_t midi_channel_max_text[32];
    GetDlgItemText(hDlg, IDC_MIDI_CHANNEL_MAX, midi_channel_max_text, 32);
    int value1 = static_cast<int>(wcstol(midi_channel_max_text, &end_ptr, 10));
    PrefSetInt(Pref::MIDI_CHANNEL_MAX, value1);
    g_gridStrument->prefMidiChannelRange(value, value1);

    wchar_t grid_size_text[32];
    GetDlgItemText(hDlg, IDC_GRID_SIZE, grid_size_text, 32);
    value = static_cast<int>(wcstol(grid_size_text, &end_ptr, 10));
    g_gridStrument->prefGridSize(value);
    PrefSetInt(Pref::GRID_SIZE, value);

    bool channel_per_row_mode = IsDlgButtonChecked(hDlg, IDC_CHANNEL_PER_ROW_MODE);
    g_gridStrument->prefChannelPerRowMode(channel_per_row_mode);
    PrefSetInt(Pref::CHANNEL_PER_ROW_MODE, channel_per_row_mode);

    bool hex_grid_mode = IsDlgButtonChecked(hDlg, IDC_HEX_GRID_MODE);
    g_gridStrument->prefHexGridMode(hex_grid_mode);
    PrefSetInt(Pref::HEX_GRID_MODE, hex_grid_mode);

    HWND midiDeviceComboBox = GetDlgItem(hDlg, IDC_MIDI_DEV_COMBO);
    int midi_device = static_cast<int>(SendMessage(midiDeviceComboBox, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
    if (g_midiDeviceIndex != midi_device) {
        assert(static_cast<unsigned int>(midi_device) < g_midiDeviceNames.size());
        // close current, open new midi device
        StopMidi();
        g_midiDeviceIndex = midi_device;
        StartMidi();
        g_gridStrument->midiDevice(g_midiDevice);
    }
    PrefSetInt(Pref::MIDI_DEVICE_INDEX, g_midiDeviceIndex);

    HWND colorThemeComboBox = GetDlgItem(hDlg, IDC_COLOR_THEME_COMBO);
    int color_theme = static_cast<int>(SendMessage(colorThemeComboBox, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
    if (static_cast<int>(g_gridStrument->prefColorTheme()) != color_theme) {
        g_gridStrument->prefColorTheme(static_cast<Theme>(color_theme));
    }
    PrefSetInt(Pref::COLOR_THEME, color_theme);

    g_dirty_main_window = true; // FIXME hack!
}

// ======================================================================
// create g_d2dRenderTarget
//
HRESULT CreateGraphicsResources(HWND hWnd)
{
    HRESULT hr = S_OK;
    // make g_d2dRenderTarget
    if (g_d2dRenderTarget == NULL) {
        RECT rc;
        GetClientRect(hWnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        hr = g_d2dFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(hWnd, size),
            &g_d2dRenderTarget);

    }
    // make g_pTextFormat
    if (g_textFormat == NULL) {
        static const WCHAR msc_fontName[] = L"Arial";
        static const FLOAT msc_fontSize = 14;

        // Create a DirectWrite text format object.
        hr = g_dwriteFactory->CreateTextFormat(
            msc_fontName,
            NULL,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            msc_fontSize,
            L"", //locale
            &g_textFormat
            );

        if (SUCCEEDED(hr)) {
            g_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
            g_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        }
    }
    return hr;
}

// ======================================================================
// release g_d2dRenderTarget
//
void DiscardGraphicsResources()
{
    SafeRelease(&g_d2dRenderTarget);
}

// ======================================================================
// resize window, so adjust the gridStrument
//
void OnResize(HWND hWnd) {
    RECT rc;
    GetClientRect(hWnd, &rc);
    D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);
    g_gridStrument->resize(size);
    if (g_d2dRenderTarget != NULL) {
        g_d2dRenderTarget->Resize(size);
        InvalidateRect(hWnd, NULL, FALSE);
    }
}

// ======================================================================
// draw the gridStrument
//
void OnPaint(HWND hWnd) {
    HRESULT hr = CreateGraphicsResources(hWnd);
    if (SUCCEEDED(hr)) {
        PAINTSTRUCT ps;
        BeginPaint(hWnd, &ps);

        g_d2dRenderTarget->BeginDraw();

        g_gridStrument->draw(g_d2dRenderTarget, g_textFormat);

        hr = g_d2dRenderTarget->EndDraw();
        if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET) {
            DiscardGraphicsResources();
        }
        EndPaint(hWnd, &ps);
    }
}

// ======================================================================
// user's finger touched screen for first time.  Gather
// the id, location, contact rectangle & pressure and tell gridStrument.
//
void OnPointerDownHandler(HWND hWnd, const POINTER_TOUCH_INFO& pti)
{
    int id = pti.pointerInfo.pointerId;
    POINT xy = pti.pointerInfo.ptPixelLocation;
    RECT r = pti.rcContact;
    ScreenToClient(hWnd, &xy);
    ScreenToClient(hWnd, &r);
    g_gridStrument->pointerDown(id, r, xy, pti.pressure);
    InvalidateRect(hWnd, NULL, FALSE);
}

// ======================================================================
// user's finger has moved/updated.  Gather info & tell gridStrument.
//
void OnPointerUpdateHandler(HWND hWnd, const POINTER_TOUCH_INFO& pti)
{
    int id = pti.pointerInfo.pointerId;
    POINT xy = pti.pointerInfo.ptPixelLocation;
    RECT r = pti.rcContact;
    ScreenToClient(hWnd, &xy);
    ScreenToClient(hWnd, &r);
    g_gridStrument->pointerUpdate(id, r, xy, pti.pressure);
    InvalidateRect(hWnd, NULL, FALSE);
}

// historical code to handle pen (not finger) events.  Not using for now
#if 0
void OnPointerUpdateHandler(HWND hWnd, const POINTER_PEN_INFO& ppi)
{
    int id = ppi.pointerInfo.pointerId;
    POINT xy = ppi.pointerInfo.ptPixelLocation;
    RECT r = { xy.x - 5, xy.y - 5, xy.x + 5, xy.y + 5 };
    ScreenToClient(hWnd, &xy);
    ScreenToClient(hWnd, &r);
    if ((ppi.pointerInfo.pointerFlags & POINTER_FLAG_NEW) == 0) {
#ifndef NDEBUG
        bool found = false;
        for (auto pair : g_gridPointers) {
            if (pair.first == id) {
                found = true;
            }
        }
        assert(found);
#endif
        auto& p = g_gridPointers[id];
        p.update(r, xy, ppi.pressure);
    }
    else {
        // FIXME - remove the previous Pen gridPointer.  It is being replaced
        // (assuming 1 pen per system)
        std::wcout << "pen id=" << id << " NEW!" << std::endl;
        g_gridPointers.emplace(id, GridPointer(id, r, xy, ppi.pressure));
    }
#ifndef NDEBUG
    // seems that ppi.pressure is 0..1024 for pens.
    std::cout << "pen id=" << id << " pressure=" << ppi.pressure << std::endl;
#endif
    InvalidateRect(hWnd, NULL, FALSE);
}
#endif

// ======================================================================
// user's fingar has lifted, tell gridStrument
//
void OnPointerUpHandler(HWND hWnd, const POINTER_TOUCH_INFO& pti)
{
    int id = pti.pointerInfo.pointerId;
    g_gridStrument->pointerUp(id);
    InvalidateRect(hWnd, NULL, FALSE);
}

// ======================================================================
// surprised this doesn't already exist.  Helper function for RECT, using
// existing POINT function.
//
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

// ======================================================================
// Ask OS which MIDI output devices are connected, store in g_midiDeviceNames
//
void QueryMidiDevices()
{
    g_midiDeviceNames.clear();
    UINT numMidiDevices = midiOutGetNumDevs();
    for (unsigned int i = 0; i < numMidiDevices; i++) {
        MIDIOUTCAPS caps;
        MMRESULT rc = midiOutGetDevCaps(i, &caps, sizeof(MIDIOUTCAPS));
        if (rc != MMSYSERR_NOERROR) {
            std::wostringstream text;
            text << "Reading midiOutGetDevCaps for #" << i << " returned=" << rc;
            AlertExit(NULL, text.str().c_str());
        }
        else {
            g_midiDeviceNames.push_back(caps.szPname);
            std::wcout << "device " << i << " is " << caps.szPname << std::endl;
        }
    }
}

// ======================================================================
// query devices and open the g_midiDeviceIndex preference, save in
// g_midiDevice.
//
MMRESULT StartMidi()
{
    // Query number of midi devices
    QueryMidiDevices();

    // Open the MIDI output port
    MMRESULT rc = midiOutOpen(&g_midiDevice, g_midiDeviceIndex, 0, 0, CALLBACK_NULL);
    if (rc != MMSYSERR_NOERROR) {
        std::wostringstream text;
        text << "Unable to midiOutOpen index=" << g_midiDeviceIndex << " returned=" << rc;
        AlertExit(NULL, text.str().c_str());
        // FIXME - try reset to device 0 if fail with higher value.
    }
    return rc;
}

// ======================================================================
// reset and close the current g_midiDevice
void StopMidi()
{
    // turn off any MIDI notes and close down.
    MMRESULT rc = midiOutReset(g_midiDevice);
    if (rc != MMSYSERR_NOERROR) {
        std::wostringstream text;
        text << "Unable to midiOutReset returned=" << rc;
        AlertExit(NULL, text.str().c_str());
    }
    rc = midiOutClose(g_midiDevice);
    if (rc != MMSYSERR_NOERROR) {
        std::wostringstream text;
        text << "Unable to midiOutClose returned=" << rc;
        AlertExit(NULL, text.str().c_str());
    }
}

// ======================================================================
// helper for all Pref enum default values.
//
int PrefGetDefault(Pref key)
{
    int value = -1;
    switch (key) {
    case Pref::GUITAR_MODE:
        value = 0;
        break;
    case Pref::MIDI_DEVICE_INDEX:
        value = 0;
        break;
    case Pref::PITCH_BEND_RANGE:
        value = 12;
        break;
    case Pref::PITCH_BEND_MASK:
        value = 0x3fff;
        break;
    case Pref::MODULATION_CONTROLLER:
        value = 1;
        break;
    case Pref::MIDI_CHANNEL_MIN:
        value = 0;
        break;
    case Pref::MIDI_CHANNEL_MAX:
        value = 9;
        break;
    case Pref::GRID_SIZE:
        value = 90;
        break;
    case Pref::CHANNEL_PER_ROW_MODE:
        value = 0;
        break;
    case Pref::COLOR_THEME:
        value = 0;
        break;
    case Pref::HEX_GRID_MODE:
        value = 0;
        break;
    default:
        std::wostringstream text;
        text << "Unknown Pref::enum=" << int(key);
        AlertExit(NULL, text.str().c_str());
    }
    return value;
}

// ======================================================================
// labels for all Pref enums.
std::wstring PrefGetLabel(Pref key)
{
    std::wstring key_str = L"KEY_NOT_FOUND";
    // setup default values
    switch (key) {
    case Pref::GUITAR_MODE:
        key_str = L"GUITAR_MODE";
        break;
    case Pref::MIDI_DEVICE_INDEX:
        key_str = L"MIDI_DEVICE_INDEX";
        break;
    case Pref::PITCH_BEND_RANGE:
        key_str = L"PITCH_BEND_RANGE";
        break;
    case Pref::PITCH_BEND_MASK:
        key_str = L"PITCH_BEND_MASK";
        break;
    case Pref::MODULATION_CONTROLLER:
        key_str = L"MODULATION_CONTROLLER";
        break;
    case Pref::MIDI_CHANNEL_MIN:
        key_str = L"MIDI_CHANNEL_MIN";
        break;
    case Pref::MIDI_CHANNEL_MAX:
        key_str = L"MIDI_CHANNEL_MAX";
        break;
    case Pref::GRID_SIZE:
        key_str = L"GRID_SIZE";
        break;
    case Pref::CHANNEL_PER_ROW_MODE:
        key_str = L"CHANNEL_PER_ROW_MODE";
        break;
    case Pref::COLOR_THEME:
        key_str = L"COLOR_THEME";
        break;
    case Pref::HEX_GRID_MODE:
        key_str = L"HEX_GRID_MODE";
        break;
    default:
        std::wostringstream text;
        text << "Unknown Pref::enum=" << int(key);
        AlertExit(NULL, text.str().c_str());
    }
    return key_str;
}

// ======================================================================
// get DWORD value from Windows Registry stored in
// HKEY_CURRENT_USER\Software\GridStrument\<key>
//
int PrefGetInt(Pref key) {
    int value = PrefGetDefault(key);
    std::wstring key_str = PrefGetLabel(key);

    HKEY hKey;
    LONG rs = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\GridStrument", 0, KEY_READ, &hKey);
    if (rs != ERROR_SUCCESS) {
        std::wcout << "PrefGetInt key=" << key_str << " default=" << value << std::endl;
        // return default
        return value;
    }

    DWORD regValueSize(sizeof(DWORD));
    DWORD regValue(0);
    rs = RegQueryValueEx(hKey, key_str.c_str(), 0, NULL,
        reinterpret_cast<LPBYTE>(&regValue),
        &regValueSize);
    if (rs == ERROR_SUCCESS) {
        std::wcout << "PrefGetInt key=" << key_str << " value=" << value << std::endl;
        // return registry value
        value = regValue;
    }
    rs = RegCloseKey(hKey);
    if (rs != ERROR_SUCCESS) {
        std::wostringstream text;
        text << "Unable to RegCloseKey returned=" << rs;
        AlertExit(NULL, text.str().c_str());
    }
    return value;
}

// ======================================================================
// set DWORD value in Windows Registry stored in
// HKEY_CURRENT_USER\Software\GridStrument\<key>
//
void PrefSetInt(Pref key, int value) {
    std::wstring key_str = PrefGetLabel(key);

    HKEY hKey;
    LONG rs = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\GridStrument", 0, KEY_ALL_ACCESS, &hKey);
    if (rs == ERROR_FILE_NOT_FOUND) {
        DWORD disposition;
        rs = RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\GridStrument", 0, 0, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, 0, &hKey, &disposition);
        assert((disposition == REG_CREATED_NEW_KEY) ||
        (disposition == REG_OPENED_EXISTING_KEY));
        if (rs != ERROR_SUCCESS) {
            std::wostringstream text;
            text << "Unable to RegCreateKeyEx returned=" << rs;
            AlertExit(NULL, text.str().c_str());
        }
    }
    else if (rs != ERROR_SUCCESS) {
        std::wostringstream text;
        text << "Unable to RegOpenKeyEx returned=" << rs;
        AlertExit(NULL, text.str().c_str());
    }

    DWORD dValue = static_cast<DWORD>(value);
    rs = RegSetValueEx(hKey, key_str.c_str(), NULL, REG_DWORD, (const BYTE*)&dValue, sizeof(dValue));
    if (rs != ERROR_SUCCESS) {
        std::wostringstream text;
        text << "Unable to RegSetValueEx returned=" << rs;
        AlertExit(NULL, text.str().c_str());
    }
    rs = RegCloseKey(hKey);
    if (rs != ERROR_SUCCESS) {
        std::wostringstream text;
        text << "Unable to RegCloseKey returned=" << rs;
        AlertExit(NULL, text.str().c_str());
    }
}
