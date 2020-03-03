#pragma once
#include <d2d1.h>
#include <mmsystem.h>
#include <map>
#include "GridPointer.h"

class GridStrument
{
    std::map<int, GridPointer> grid_pointers_;
    std::map<int, int> grid_notes_;
    D2D1_SIZE_U size_;
    int num_grids_x_, num_grids_y_;
    HMIDIOUT midi_device_;
    ID2D1SolidColorBrush* grid_line_brush_;
    ID2D1SolidColorBrush* c_note_brush_;
    ID2D1SolidColorBrush* note_brush_;
public:
    GridStrument(HMIDIOUT midiDevice);
    void Resize(D2D1_SIZE_U size);
    void Draw(ID2D1HwndRenderTarget* d2dRenderTarget);
    void PointerDown(int id, RECT rect, POINT point, int pressure);
    void PointerUpdate(int id, RECT rect, POINT point, int pressure);
    void PointerUp(int id);
private:
    int PointToMidiNote(POINT point);
    int GridLocToMidiNote(int x, int y);
};

