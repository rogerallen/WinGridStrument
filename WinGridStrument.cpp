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

#include "framework.h"
#include "WinGridStrument.h"
#include "GridStrument.h"
#include "GridUtils.h"

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

const static int MAX_LOADSTRING = 100;
enum class Pref { 
    MIDI_DEVICE_INDEX, GUITAR_MODE, PITCH_BEND_RANGE, PITCH_BEND_MASK,
    MODULATION_CONTROLLER, MIDI_CHANNEL_MIN, MIDI_CHANNEL_MAX, 
    GRID_SIZE, CHANNEL_PER_ROW_MODE
};

// Global Variables:
HINSTANCE g_instance;

// D2D vars -- really globals?
ID2D1Factory* g_d2dFactory;
ID2D1HwndRenderTarget* g_d2dRenderTarget;

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

HRESULT CreateGraphicsResources(HWND);
void DiscardGraphicsResources();
void OnResize(HWND hWnd);
void OnPaint(HWND hWnd);

void OnPointerDownHandler(HWND hWnd, const POINTER_TOUCH_INFO& pti);
void OnPointerUpdateHandler(HWND hWnd, const POINTER_TOUCH_INFO& pti);
//void OnPointerUpdateHandler(HWND hWnd, const POINTER_PEN_INFO& ppi);
void OnPointerUpHandler(HWND hWnd, const POINTER_TOUCH_INFO& pti);

void ScreenToClient(HWND hWnd, RECT* r);

MMRESULT StartMidi();
void QueryMidiDevices();
void StopMidi();

int PrefGetInt(Pref key);
void PrefSetInt(Pref key, int value);

void AlertExit(HWND hWnd, LPCTSTR text);


// ======================================================================
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
    g_gridStrument->PrefGuitarMode(PrefGetInt(Pref::GUITAR_MODE));
    g_gridStrument->PrefPitchBendRange(PrefGetInt(Pref::PITCH_BEND_RANGE));
    g_gridStrument->PrefPitchBendMask(PrefGetInt(Pref::PITCH_BEND_MASK));
    g_gridStrument->PrefModulationController(PrefGetInt(Pref::MODULATION_CONTROLLER));
    g_gridStrument->PrefMidiChannelRange(PrefGetInt(Pref::MIDI_CHANNEL_MIN), PrefGetInt(Pref::MIDI_CHANNEL_MAX));
    g_gridStrument->PrefGridSize(PrefGetInt(Pref::GRID_SIZE));
    g_gridStrument->PrefChannelPerRowMode(PrefGetInt(Pref::CHANNEL_PER_ROW_MODE));

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
        if (g_dirty_main_window) {
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
//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance, WCHAR *windowClass)
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
void InitInstance(HINSTANCE hInstance, int nCmdShow, WCHAR *windowClass, WCHAR *title)
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
        } // we do not get (pointer_type == PT_PEN) 
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
#if 0
        else if (pointer_type == PT_PEN) {
            // all the events for PEN go through here
            POINTER_PEN_INFO ppi;
            GetPointerPenInfo(GET_POINTERID_WPARAM(wParam), &ppi);
            OnPointerUpdateHandler(hWnd, ppi);
        }
#endif
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
        } // don't get these (pointer_type == PT_PEN)
    }
    break;
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
    }
    break;
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
    case WM_INITDIALOG:
    {
        HWND midiDeviceComboBox = GetDlgItem(hDlg, IDC_MIDI_DEV_COMBO);
        for (auto s : g_midiDeviceNames) {
            SendMessage(midiDeviceComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)s.c_str());
        }
        SendMessage(midiDeviceComboBox, CB_SETCURSEL, (WPARAM)g_midiDeviceIndex, (LPARAM)0);

        CheckDlgButton(hDlg, IDC_GUITAR_MODE, g_gridStrument->PrefGuitarMode());

        int value = g_gridStrument->PrefPitchBendRange();
        std::wstring tmp_str = std::to_wstring(value);
        SetDlgItemText(hDlg, IDC_PITCH_BEND_RANGE, tmp_str.c_str());

        value = g_gridStrument->PrefPitchBendMask();
        std::wstringstream tmp_ss;
        tmp_ss << L"0x" << std::hex << value << std::dec;
        SetDlgItemText(hDlg, IDC_PITCH_BEND_MASK, tmp_ss.str().c_str());
        
        value = g_gridStrument->PrefModulationController();
        tmp_str = std::to_wstring(value);
        SetDlgItemText(hDlg, IDC_MODULATION_CONTROLLER, tmp_str.c_str());
        
        value = g_gridStrument->PrefMidiChannelMin();
        tmp_str = std::to_wstring(value);
        SetDlgItemText(hDlg, IDC_MIDI_CHANNEL_MIN, tmp_str.c_str());

        value = g_gridStrument->PrefMidiChannelMax();
        tmp_str = std::to_wstring(value);
        SetDlgItemText(hDlg, IDC_MIDI_CHANNEL_MAX, tmp_str.c_str());

        value = g_gridStrument->PrefGridSize();
        tmp_str = std::to_wstring(value);
        SetDlgItemText(hDlg, IDC_GRID_SIZE, tmp_str.c_str());

        CheckDlgButton(hDlg, IDC_CHANNEL_PER_ROW_MODE, g_gridStrument->PrefChannelPerRowMode());

        return (INT_PTR)TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            if (LOWORD(wParam) == IDOK) {
                bool guitar_mode = IsDlgButtonChecked(hDlg, IDC_GUITAR_MODE);
                g_gridStrument->PrefGuitarMode(guitar_mode);
                PrefSetInt(Pref::GUITAR_MODE, guitar_mode);
                
                wchar_t pitch_range_text[32];
                GetDlgItemText(hDlg, IDC_PITCH_BEND_RANGE, pitch_range_text, 32);
                wchar_t* end_ptr;
                int value = static_cast<int>(wcstol(pitch_range_text, &end_ptr, 10));
                g_gridStrument->PrefPitchBendRange(value);
                PrefSetInt(Pref::PITCH_BEND_RANGE, value);

                wchar_t pitch_mask_text[32];
                GetDlgItemText(hDlg, IDC_PITCH_BEND_MASK, pitch_mask_text, 32);
                value = static_cast<int>(wcstol(pitch_mask_text, &end_ptr, 16));
                g_gridStrument->PrefPitchBendMask(value);
                PrefSetInt(Pref::PITCH_BEND_MASK, value);

                wchar_t controller_text[32];
                GetDlgItemText(hDlg, IDC_MODULATION_CONTROLLER, controller_text, 32);
                value = static_cast<int>(wcstol(controller_text, &end_ptr, 10));
                g_gridStrument->PrefModulationController(value);
                PrefSetInt(Pref::MODULATION_CONTROLLER, value);

                wchar_t midi_channel_min_text[32];
                GetDlgItemText(hDlg, IDC_MIDI_CHANNEL_MIN, midi_channel_min_text, 32);
                value = static_cast<int>(wcstol(midi_channel_min_text, &end_ptr, 10));
                PrefSetInt(Pref::MIDI_CHANNEL_MIN, value);

                wchar_t midi_channel_max_text[32];
                GetDlgItemText(hDlg, IDC_MIDI_CHANNEL_MAX, midi_channel_max_text, 32);
                int value1 = static_cast<int>(wcstol(midi_channel_max_text, &end_ptr, 10));
                PrefSetInt(Pref::MIDI_CHANNEL_MAX, value1);
                g_gridStrument->PrefMidiChannelRange(value, value1);

                wchar_t grid_size_text[32];
                GetDlgItemText(hDlg, IDC_GRID_SIZE, grid_size_text, 32);
                value = static_cast<int>(wcstol(grid_size_text, &end_ptr, 10));
                g_gridStrument->PrefGridSize(value);
                PrefSetInt(Pref::GRID_SIZE, value);

                bool channel_per_row_mode = IsDlgButtonChecked(hDlg, IDC_CHANNEL_PER_ROW_MODE);
                g_gridStrument->PrefChannelPerRowMode(channel_per_row_mode);
                PrefSetInt(Pref::CHANNEL_PER_ROW_MODE, channel_per_row_mode);

                HWND midiDeviceComboBox = GetDlgItem(hDlg, IDC_MIDI_DEV_COMBO);
                int midi_device = static_cast<int>(SendMessage(midiDeviceComboBox, CB_GETCURSEL, (WPARAM)0, (LPARAM)0));
                if (g_midiDeviceIndex != midi_device) {
                    assert(midi_device < g_midiDeviceNames.size());
                    // close current, open new midi device
                    StopMidi();
                    g_midiDeviceIndex = midi_device;
                    StartMidi();
                    g_gridStrument->MidiDevice(g_midiDevice);
                }
                PrefSetInt(Pref::MIDI_DEVICE_INDEX, g_midiDeviceIndex);

                g_dirty_main_window = true; // FIXME hack!
            }
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
    if (g_d2dRenderTarget == NULL) {
        RECT rc;
        GetClientRect(hWnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        hr = g_d2dFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(hWnd, size),
            &g_d2dRenderTarget);

    }
    return hr;
}
void DiscardGraphicsResources()
{
    SafeRelease(&g_d2dRenderTarget);
}

void OnResize(HWND hWnd) {
    RECT rc;
    GetClientRect(hWnd, &rc);
    D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);
    g_gridStrument->Resize(size);
    if (g_d2dRenderTarget != NULL) {
        g_d2dRenderTarget->Resize(size);
        InvalidateRect(hWnd, NULL, FALSE);
    }
}

void OnPaint(HWND hWnd) {
    HRESULT hr = CreateGraphicsResources(hWnd);
    if (SUCCEEDED(hr)) {
        PAINTSTRUCT ps;
        BeginPaint(hWnd, &ps);

        g_d2dRenderTarget->BeginDraw();

        g_gridStrument->Draw(g_d2dRenderTarget);

        hr = g_d2dRenderTarget->EndDraw();
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
    POINT xy = pti.pointerInfo.ptPixelLocation;
    RECT r = pti.rcContact;
    ScreenToClient(hWnd, &xy);
    ScreenToClient(hWnd, &r);
    g_gridStrument->PointerDown(id, r, xy, pti.pressure);
    InvalidateRect(hWnd, NULL, FALSE);
}

// ======================================================================
void OnPointerUpdateHandler(HWND hWnd, const POINTER_TOUCH_INFO& pti)
{
    int id = pti.pointerInfo.pointerId;
    POINT xy = pti.pointerInfo.ptPixelLocation;
    RECT r = pti.rcContact;
    ScreenToClient(hWnd, &xy);
    ScreenToClient(hWnd, &r);
    g_gridStrument->PointerUpdate(id, r, xy, pti.pressure);
    InvalidateRect(hWnd, NULL, FALSE);
}

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
void OnPointerUpHandler(HWND hWnd, const POINTER_TOUCH_INFO& pti)
{
    int id = pti.pointerInfo.pointerId;
    g_gridStrument->PointerUp(id);
    InvalidateRect(hWnd, NULL, FALSE);
}

// ======================================================================
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
    }
    return rc;
}

// ======================================================================
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
int PrefGetDefault(Pref key) {
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
    default:
        std::wostringstream text;
        text << "Unknown Pref::enum=" << int(key);
        AlertExit(NULL, text.str().c_str());
    }
    return value;
}

// ======================================================================
std::wstring PrefGetLabel(Pref key) {
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
    default:
        std::wostringstream text;
        text << "Unknown Pref::enum=" << int(key);
        AlertExit(NULL, text.str().c_str());
    }
    return key_str;
}

// ======================================================================
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
