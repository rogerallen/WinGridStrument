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
#pragma once
#include <d2d1.h>
#include <mmsystem.h>
#include <algorithm>
#include <map>
#include <iostream>
#include <assert.h>
#include "GridPointer.h"
#include "GridMidi.h"

// ======================================================================
// helper class for all the brushes.  Have to construct the brushes
// after we get the render target which isn't ready when GridStrument 
// is first constructed.
// 
struct GridBrushes
{
    bool initialized_;                // have we constructed the brushes yet?
    ID2D1SolidColorBrush* grid_line_; 
    ID2D1SolidColorBrush* c_note_;
    ID2D1SolidColorBrush* note_;
    ID2D1SolidColorBrush* highlight_;
    ID2D1SolidColorBrush* guitar_;
    GridBrushes() {
        initialized_ = false;
        grid_line_ = nullptr;
        c_note_ = nullptr;
        note_ = nullptr;
        highlight_ = nullptr;
        guitar_ = nullptr;
    }
    void init(ID2D1HwndRenderTarget* d2dRenderTarget)
    {
        initialized_ = true;
        D2D1_COLOR_F color = D2D1::ColorF(0.75f, 0.75f, 0.75f);
        HRESULT hr = d2dRenderTarget->CreateSolidColorBrush(color, &grid_line_);
        assert(SUCCEEDED(hr));

        color = D2D1::ColorF(0.f, 0.f, 0.85f);
        hr = d2dRenderTarget->CreateSolidColorBrush(color, &c_note_);
        assert(SUCCEEDED(hr));
        
        color = D2D1::ColorF(0.f, 0.85f, 0.f);
        hr = d2dRenderTarget->CreateSolidColorBrush(color, &note_);
        assert(SUCCEEDED(hr));
        
        color = D2D1::ColorF(0.90f, 0.90f, 0.f);
        hr = d2dRenderTarget->CreateSolidColorBrush(color, &highlight_);
        assert(SUCCEEDED(hr));
        
        color = D2D1::ColorF(0.50f, 0.50f, 0.40f);
        hr = d2dRenderTarget->CreateSolidColorBrush(color, &guitar_);
        assert(SUCCEEDED(hr));
    }
};

class GridStrument
{
    // preferences that control how GridStrument works
    bool pref_guitar_mode_;
    int pref_pitch_bend_range_;
    int pref_pitch_bend_mask_;
    int pref_modulation_controller_;
    int pref_midi_channel_min_, pref_midi_channel_max_;
    int pref_grid_size_;
    bool pref_channel_per_row_mode_;

    // all of the current finger touches in one dictionary
    std::map<int, GridPointer> grid_pointers_;
    D2D1_SIZE_U size_;               // size of the window
    int num_grids_x_, num_grids_y_;  // number of boxes for notes
    GridMidi* midi_device_;          // the current midi output
    int midi_channel_;               // next midi channel to use
    GridBrushes brushes_;            // all of our brushes

public:
    GridStrument(HMIDIOUT midiDevice);
    void midiDevice(HMIDIOUT midiDevice);
    void resize(D2D1_SIZE_U size);
    void draw(ID2D1HwndRenderTarget* d2dRenderTarget);
    void pointerDown(int id, RECT rect, POINT point, int pressure);
    void pointerUpdate(int id, RECT rect, POINT point, int pressure);
    void pointerUp(int id);
    // get/set preferences
    bool prefGuitarMode() { return pref_guitar_mode_; }
    void prefGuitarMode(bool mode) { pref_guitar_mode_ = mode; }
    int prefPitchBendRange() { return pref_pitch_bend_range_; }
    void prefPitchBendRange(int value) {
        pref_pitch_bend_range_ = std::clamp(value, 1, 12);
    }
    int prefPitchBendMask() { return pref_pitch_bend_mask_; }
    void prefPitchBendMask(int value) {
        pref_pitch_bend_mask_ = std::clamp(value, 0x2000, 0x3fff);
    }
    int prefModulationController() { return pref_modulation_controller_; }
    void prefModulationController(int value) {
        // https://www.midi.org/specifications-old/item/table-3-control-change-messages-data-bytes-2
        pref_modulation_controller_ = std::clamp(value, 1, 119);
    }
    int prefMidiChannelMin() { return pref_midi_channel_min_; }
    int prefMidiChannelMax() { return pref_midi_channel_max_; }
    void prefMidiChannelRange(int min, int max) {
        pref_midi_channel_min_ = std::clamp(min, 0, 15);
        pref_midi_channel_max_ = std::clamp(max, 0, 15);
        if (pref_midi_channel_min_ > pref_midi_channel_max_) {
            std::wcout << "Forcing Midi Channel min == max == " << pref_midi_channel_max_ << std::endl;
            pref_midi_channel_min_ = pref_midi_channel_max_;
        }
    }
    int prefGridSize() { return pref_grid_size_; }
    void prefGridSize(int value) {
        value = std::clamp(value, 40, 400);
        if (value != pref_grid_size_) {
            pref_grid_size_ = value;
            resize(size_);
        }
    }
    bool prefChannelPerRowMode() { return pref_channel_per_row_mode_; }
    void prefChannelPerRowMode(bool mode) { pref_channel_per_row_mode_ = mode; }
private:
    void drawPointers(ID2D1HwndRenderTarget* d2dRenderTarget);
    void drawDots(ID2D1HwndRenderTarget* d2dRenderTarget);
    void drawGuitar(ID2D1HwndRenderTarget* d2dRenderTarget);
    void drawGrid(ID2D1HwndRenderTarget* d2dRenderTarget);
    void nextMidiChannel();
    int pointToGridColumn(POINT point);
    int pointToGridRow(POINT point);
    int pointToMidiNote(POINT point);
    int gridLocToMidiNote(int x, int y);
    int rectToMidiPressure(RECT rect);
    int pointChangeToPitchBend(POINT delta);
    int pointChangeToMidiModulation(POINT delta);
};

