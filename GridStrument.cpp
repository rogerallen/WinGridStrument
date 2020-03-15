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
#include "GridStrument.h"
#include "GridUtils.h"
#include <cassert>
#include <iostream>
#include <set>

GridStrument::GridStrument(HMIDIOUT midiDevice)
{
    // initial preferences
    pref_guitar_mode_ = true;
    pref_pitch_bend_range_ = 12;
    pref_modulation_controller_ = 1;
    pref_midi_channel_min_ = 0;
    pref_midi_channel_max_ = 10;
    pref_grid_size_ = 90;
    pref_channel_per_row_mode_ = false;
    pref_pitch_bend_mask_ = 0x3fff;

    size_ = D2D1::SizeU(0, 0);
    num_grids_x_ = num_grids_y_ = 0;
    midi_device_ = new GridMidi(midiDevice);
    midi_channel_ = pref_midi_channel_min_;

}

void GridStrument::MidiDevice(HMIDIOUT midiDevice) {
    free(midi_device_);
    midi_device_ = new GridMidi(midiDevice);
}

void GridStrument::Resize(D2D1_SIZE_U size)
{
    size_ = size;
    num_grids_x_ = (int)(size_.width / pref_grid_size_);
    num_grids_y_ = (int)(size_.height / pref_grid_size_);

#ifndef NDEBUG
    std::wcout << "screen width = " << size.width << ", height = " << size.height << std::endl;
    std::wcout << "screen columns = " << num_grids_x_ << ", rows = " << num_grids_y_ << std::endl;
    std::wcout << "min note = " << GridLocToMidiNote(0, num_grids_y_ - 1) << std::endl;
    std::wcout << "max note = " << GridLocToMidiNote(num_grids_x_ - 1, 0) << std::endl;
#endif // !NDEBUG
}

void GridStrument::Draw(ID2D1HwndRenderTarget* d2dRenderTarget)
{
    if (!brushes_.initialized) {
        brushes_.Init(d2dRenderTarget);
    }
    d2dRenderTarget->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
    if (pref_guitar_mode_) {
        DrawGuitar(d2dRenderTarget);
    }
    DrawGrid(d2dRenderTarget);
    DrawDots(d2dRenderTarget);
    DrawPointers(d2dRenderTarget);
}

void GridStrument::DrawPointers(ID2D1HwndRenderTarget* d2dRenderTarget)
{
    for (auto p : grid_pointers_) {
        ID2D1SolidColorBrush* pBrush;
        float pressure = p.second.pressure() / 512.0f;
        const D2D1_COLOR_F color = D2D1::ColorF(pressure, 0.0f, pressure, 0.5f);
        HRESULT hr = d2dRenderTarget->CreateSolidColorBrush(color, &pBrush);
        if (SUCCEEDED(hr)) {
            RECT rc = p.second.rect();
            D2D1_RECT_F rcf = D2D1::RectF((float)rc.left, (float)rc.top, (float)rc.right, (float)rc.bottom);
            d2dRenderTarget->FillRectangle(&rcf, pBrush);
            SafeRelease(&pBrush);
        }
    }
}

void GridStrument::DrawDots(ID2D1HwndRenderTarget* d2dRenderTarget)
{
    std::set<int> active_notes;
    for (auto p : grid_pointers_) {
        active_notes.insert(p.second.note());
    }
    for (int x = 0; x < num_grids_x_; x++) {
        for (int y = 0; y < num_grids_y_; y++) {
            int note = GridLocToMidiNote(x, y);
            D2D1_ELLIPSE ellipse = D2D1::Ellipse(
                D2D1::Point2F(x * pref_grid_size_ + pref_grid_size_ / 2.f, y * pref_grid_size_ + pref_grid_size_ / 2.f),
                pref_grid_size_ / 5.f,
                pref_grid_size_ / 5.f
            );
            if (active_notes.find(note) != active_notes.end()) {
                d2dRenderTarget->FillEllipse(ellipse, brushes_.highlight_);
            }
            else if (note % 12 == 0) {
                d2dRenderTarget->FillEllipse(ellipse, brushes_.c_note_);
            }
            else if (((note % 12) == 2) || ((note % 12) == 4) || ((note % 12) == 5) ||
                ((note % 12) == 7) || ((note % 12) == 9) || ((note % 12) == 11)) {
                d2dRenderTarget->FillEllipse(ellipse, brushes_.note_);

            }
        }
    }
}

void GridStrument::DrawGuitar(ID2D1HwndRenderTarget* d2dRenderTarget)
{
    float left, right, top, bottom;
    left = 0.0f;
    right = 1.0f * num_grids_x_ * pref_grid_size_;
    top = 1.0f * (num_grids_y_ / 2 + 4) * pref_grid_size_;
    bottom = 1.0f * (num_grids_y_ / 2 - 2) * pref_grid_size_;
    if (num_grids_y_ % 2 == 0) {
        top -= 1.0f * pref_grid_size_;
        bottom -= 1.0f * pref_grid_size_;
    }
    D2D1_RECT_F rcf = D2D1::RectF(left, top, right, bottom);
    d2dRenderTarget->FillRectangle(&rcf, brushes_.guitar_);
}

void GridStrument::DrawGrid(ID2D1HwndRenderTarget* d2dRenderTarget)
{
    for (int x = 0; x < num_grids_x_ * pref_grid_size_ + 1; x += pref_grid_size_) {
        d2dRenderTarget->DrawLine(
            D2D1::Point2F(static_cast<FLOAT>(x), 0.0f),
            D2D1::Point2F(static_cast<FLOAT>(x), static_cast<FLOAT>(num_grids_y_ * pref_grid_size_)),
            brushes_.grid_line_,
            1.5f
        );
    }
    for (int y = 0; y < num_grids_y_ * pref_grid_size_ + 1; y += pref_grid_size_) {
        d2dRenderTarget->DrawLine(
            D2D1::Point2F(0.0f, static_cast<FLOAT>(y)),
            D2D1::Point2F(static_cast<FLOAT>(num_grids_x_ * pref_grid_size_), static_cast<FLOAT>(y)),
            brushes_.grid_line_,
            1.5f
        );
    }
}

void GridStrument::PointerDown(int id, RECT rect, POINT point, int pressure)
{
#ifndef NDEBUG
    for (auto pair : grid_pointers_) {
        assert(pair.first != id);
    }
#endif  
    grid_pointers_.emplace(id, GridPointer(id, rect, point, pressure));
    int note = PointToMidiNote(point);
    grid_pointers_[id].note(note);
    // assume default midi channel mode
    int channel = midi_channel_;
    NextMidiChannel();
    // override in the case of per-row-mode
    if (pref_channel_per_row_mode_) {
        // use as many channels as you can, given min/max range
        int row = PointToGridRow(point);
        channel = row % (pref_midi_channel_max_ + 1 - pref_midi_channel_min_);
        channel += pref_midi_channel_min_;
        midi_channel_ = channel;
    }
    grid_pointers_[id].channel(channel);
    int midi_pressure = RectToMidiPressure(rect);
    grid_pointers_[id].modulationZ(midi_pressure);
    grid_pointers_[id].modulationX(-1);
    grid_pointers_[id].modulationY(-1);
    if (note >= 0) {
        midi_device_->noteOn(channel, note, midi_pressure);

    }
}

void GridStrument::NextMidiChannel() {
    midi_channel_++;
    if (midi_channel_ > pref_midi_channel_max_) {
        midi_channel_ = pref_midi_channel_min_;
    }
}

void GridStrument::PointerUpdate(int id, RECT rect, POINT point, int pressure)
{
    // NOTE: seems that pressure is always 512 for fingers.
#ifndef NDEBUG
    bool found = false;
    for (auto pair : grid_pointers_) {
        if (pair.first == id) {
            found = true;
        }
    }
    assert(found);
#endif
    auto& cur_ptr = grid_pointers_[id];
    cur_ptr.update(rect, point, pressure);
    int channel = cur_ptr.channel();
    POINT change = cur_ptr.pointChange();
    int mod_pitch = PointChangeToMidiPitch(change);
    if (mod_pitch != cur_ptr.modulationX()) {
        cur_ptr.modulationX(mod_pitch);
        midi_device_->pitchBend(channel, mod_pitch);
    }
    int mod_modulation = PointChangeToMidiModulation(change);
    if (mod_modulation != cur_ptr.modulationY()) {
        cur_ptr.modulationY(mod_modulation);
        midi_device_->controlChange(channel, pref_modulation_controller_, mod_modulation);
    }
    int midi_pressure = RectToMidiPressure(rect);
    if (midi_pressure != cur_ptr.modulationZ()) {
        cur_ptr.modulationZ(midi_pressure);
        int note = cur_ptr.note();
        midi_device_->polyKeyPressure(channel, note, midi_pressure);
    }
}

void GridStrument::PointerUp(int id)
{
#ifndef NDEBUG
    bool found = false;
    for (auto pair : grid_pointers_) {
        if (pair.first == id) {
            found = true;
        }
    }
    assert(found);
#endif
    int note = grid_pointers_[id].note();
    int channel = grid_pointers_[id].channel();
    grid_pointers_.erase(id);
    if (note >= 0) {
        midi_device_->noteOn(channel, note, 0);
    }
    midi_device_->controlChange(channel, pref_modulation_controller_, 0);
}

int GridStrument::PointToGridColumn(POINT point) {
    if (point.x > num_grids_x_* pref_grid_size_) {
        return -1;
    }
    int x = point.x / pref_grid_size_;
    return x;
}
int GridStrument::PointToGridRow(POINT point) {
    if (point.y > num_grids_y_* pref_grid_size_) {
        return -1;
    }
    int y = point.y / pref_grid_size_;
    return y;
}
int GridStrument::PointToMidiNote(POINT point)
{
    int x = PointToGridColumn(point);
    int y = PointToGridRow(point);
    int note = GridLocToMidiNote(x, y);
    return note;
}

int GridStrument::GridLocToMidiNote(int x, int y)
{
    // Y-Delta is like the X row size in normal array index math
    // It is fixed at 5 to match going up by musical fourths
    int Y_DELTA = 5;
    // Y-invert so up is higher note.  Grid Y ranges from 0..(num-1)
    y = num_grids_y_ - 1 - y;
    // put 55 or guitar's 4th string open G on left in middle
    int center_y = num_grids_y_ / 2;
    int offset = 55 - (0 + center_y * Y_DELTA);
    // do guitar-type string change at 59 instead of 60
    // for the B-string (rather than C) offset
    if (pref_guitar_mode_) {
        if (y > center_y) {
            offset -= 1;
        }
    }
    int note = offset + x + y * Y_DELTA;
    note = std::clamp(note, 0, 127);
    return note;
}

int GridStrument::RectToMidiPressure(RECT rect)
{
    int x = (rect.right - rect.left);
    int y = (rect.bottom - rect.top);
    int area = x * y;
    float ratio = static_cast<float>(area) / (pref_grid_size_ / 2 * pref_grid_size_ / 2);
    int pressure = static_cast<int>(ratio * 70);
    if (pressure > 127) {
        pressure = 127;
    }
    return pressure;
}

int GridStrument::PointChangeToMidiPitch(POINT delta)
{
    int dx = delta.x;
    float range = 1.0f * pref_pitch_bend_range_;
    int pitch = 0x2000 + static_cast<int>(0x2000 * (dx / (range * pref_grid_size_)));
    if (pitch > 0x3fff) {
        pitch = 0x3fff;
    }
    else if (pitch < 0) {
        pitch = 0;
    }
    // this can be used to mask off low bits
    pitch = pitch & pref_pitch_bend_mask_;
    return pitch;
}

int GridStrument::PointChangeToMidiModulation(POINT delta)
{
    int dy = abs(delta.y);
    int modulation = static_cast<int>(0x7f * (dy / (1.0f * pref_grid_size_)));
    if (modulation > 0x7f) {
        modulation = 0x7f;
    }
    else if (modulation < 0) {
        modulation = 0;
    }
    return modulation;
}