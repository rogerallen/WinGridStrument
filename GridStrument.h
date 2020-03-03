#pragma once
#include <d2d1.h>
#include <mmsystem.h>
#include <map>
#include "GridPointer.h"

class GridStrument
{
    std::map<int, GridPointer> grid_pointers_;
    D2D1_SIZE_U size_;
    HMIDIOUT midi_device_;
public:
    GridStrument(HMIDIOUT midiDevice);
    void Resize(D2D1_SIZE_U size);
    void Draw(ID2D1HwndRenderTarget* d2dRenderTarget);
    void PointerDown(int id, RECT rect, POINT point, int pressure);
    void PointerUpdate(int id, RECT rect, POINT point, int pressure);
    void PointerUp(int id);
};

