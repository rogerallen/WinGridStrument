// ======================================================================
// WinGridStrument - a Windows touchscreen musical instrument
// Copyright(C) 2020 Roger Allen
// 
// This program is free software : you can redistribute itand /or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.If not, see < https://www.gnu.org/licenses/>.
// ======================================================================
#pragma once
#include <d2d1.h>
#include <mmsystem.h>
#include <map>
#include "GridPointer.h"
#include "GridMidi.h"

class GridStrument
{
    bool pref_guitar_mode_;
    std::map<int, GridPointer> grid_pointers_;
    D2D1_SIZE_U size_;
    int num_grids_x_, num_grids_y_;
    GridMidi* midi_device_;
    ID2D1SolidColorBrush* grid_line_brush_;
    ID2D1SolidColorBrush* c_note_brush_;
    ID2D1SolidColorBrush* note_brush_;
    ID2D1SolidColorBrush* highlight_brush_;
public:
    GridStrument(HMIDIOUT midiDevice);
    void Resize(D2D1_SIZE_U size);
    void Draw(ID2D1HwndRenderTarget* d2dRenderTarget);
    void PointerDown(int id, RECT rect, POINT point, int pressure);
    void PointerUpdate(int id, RECT rect, POINT point, int pressure);
    void PointerUp(int id);
    bool PrefGuitarMode() { return pref_guitar_mode_; };
    void PrefGuitarMode(bool mode) { pref_guitar_mode_ = mode; };
private:
    int PointToMidiNote(POINT point);
    int GridLocToMidiNote(int x, int y);
    int RectToMidiPressure(RECT rect);
    int PointChangeToMidiPitch(POINT change);
    int PointChangeToMidiModulation(POINT delta);
};

