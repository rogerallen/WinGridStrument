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
#include <algorithm>
#include <map>
#include <iostream>
#include "GridPointer.h"
#include "GridMidi.h"

class GridStrument
{
    bool pref_guitar_mode_;
    int pref_pitch_bend_range_;
    int pref_modulation_controller_;
    int pref_midi_channel_min_, pref_midi_channel_max_;


    std::map<int, GridPointer> grid_pointers_;
    D2D1_SIZE_U size_;
    int num_grids_x_, num_grids_y_;
    GridMidi* midi_device_;
    int midi_channel_;
    ID2D1SolidColorBrush* grid_line_brush_;
    ID2D1SolidColorBrush* c_note_brush_;
    ID2D1SolidColorBrush* note_brush_;
    ID2D1SolidColorBrush* highlight_brush_;
public:
    GridStrument(HMIDIOUT midiDevice);
    void MidiDevice(HMIDIOUT midiDevice);
    void Resize(D2D1_SIZE_U size);
    void Draw(ID2D1HwndRenderTarget* d2dRenderTarget);
    void DrawPointers(ID2D1HwndRenderTarget* d2dRenderTarget);
    void DrawDots(ID2D1HwndRenderTarget* d2dRenderTarget);
    void DrawGrid(ID2D1HwndRenderTarget* d2dRenderTarget);
    void InitBrushes(ID2D1HwndRenderTarget* d2dRenderTarget);
    void PointerDown(int id, RECT rect, POINT point, int pressure);
    void NextMidiChannel();
    void PointerUpdate(int id, RECT rect, POINT point, int pressure);
    void PointerUp(int id);
    bool PrefGuitarMode() { return pref_guitar_mode_; };
    void PrefGuitarMode(bool mode) { pref_guitar_mode_ = mode; };
    int PrefPitchBendRange() { return pref_pitch_bend_range_; };
    void PrefPitchBendRange(int value) {
        pref_pitch_bend_range_ = std::clamp(value, 1, 12);
    }
    int PrefModulationController() { return pref_modulation_controller_; };
    void PrefModulationController(int value) {
        // https://www.midi.org/specifications-old/item/table-3-control-change-messages-data-bytes-2
        pref_modulation_controller_ = std::clamp(value, 1, 119);
    }
    int PrefMidiChannelMin() { return pref_midi_channel_min_; };
    int PrefMidiChannelMax() { return pref_midi_channel_max_; };
    void PrefMidiChannelRange(int min, int max) {
        pref_midi_channel_min_ = std::clamp(min, 0, 15);
        pref_midi_channel_max_ = std::clamp(max, 0, 15);
        if (pref_midi_channel_min_ > pref_midi_channel_max_) {
            std::wcout << "Forcing Midi Channel min == max == " << pref_midi_channel_max_ << std::endl;
            pref_midi_channel_min_ = pref_midi_channel_max_;
        }
    }
private:
    int PointToMidiNote(POINT point);
    int GridLocToMidiNote(int x, int y);
    int RectToMidiPressure(RECT rect);
    int PointChangeToMidiPitch(POINT change);
    int PointChangeToMidiModulation(POINT delta);
};

