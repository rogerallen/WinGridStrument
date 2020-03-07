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
#include "GridStrument.h"
#include "GridUtils.h"
#include <cassert>
#include <iostream>

static const int GRID_SIZE = 90; // FIXME - prefs

GridStrument::GridStrument(HMIDIOUT midiDevice)
{
    // initial preferences
    pref_guitar_mode_ = true;

    size_ = D2D1::SizeU(0, 0);
    num_grids_x_ = num_grids_y_ = 0;
    midi_device_ = new GridMidi(midiDevice);
    grid_line_brush_ = nullptr;
    c_note_brush_ = nullptr;
    note_brush_ = nullptr;
    highlight_brush_ = nullptr;
}

void GridStrument::Resize(D2D1_SIZE_U size)
{
    size_ = size;
    num_grids_x_ = (int)(size_.width / GRID_SIZE);
    num_grids_y_ = (int)(size_.height / GRID_SIZE);

#ifndef NDEBUG
    std::wcout << "screen width = " << size.width << ", height = " << size.height << std::endl;
    std::wcout << "screen columns = " << num_grids_x_ << ", rows = " << num_grids_y_ << std::endl;
    std::wcout << "min note = " << GridLocToMidiNote(0, num_grids_y_ - 1) << std::endl;
    std::wcout << "max note = " << GridLocToMidiNote(num_grids_x_ - 1, 0) << std::endl;
#endif // !NDEBUG
}

void GridStrument::Draw(ID2D1HwndRenderTarget* d2dRenderTarget)
{
    if (grid_line_brush_ == nullptr) {
        D2D1_COLOR_F color = D2D1::ColorF(0.75f, 0.75f, 0.75f);
        HRESULT hr = d2dRenderTarget->CreateSolidColorBrush(color, &grid_line_brush_);
        assert(SUCCEEDED(hr));
        color = D2D1::ColorF(0.f, 0.f, 0.85f);
        hr = d2dRenderTarget->CreateSolidColorBrush(color, &c_note_brush_);
        assert(SUCCEEDED(hr));
        color = D2D1::ColorF(0.f, 0.85f, 0.f);
        hr = d2dRenderTarget->CreateSolidColorBrush(color, &note_brush_);
        assert(SUCCEEDED(hr));
        color = D2D1::ColorF(0.85f, 0.85f, 0.f);
        hr = d2dRenderTarget->CreateSolidColorBrush(color, &highlight_brush_);
        assert(SUCCEEDED(hr));
    }

    d2dRenderTarget->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

    for (int x = 0; x < num_grids_x_ * GRID_SIZE + 1; x += GRID_SIZE) {
        d2dRenderTarget->DrawLine(
            D2D1::Point2F(static_cast<FLOAT>(x), 0.0f),
            D2D1::Point2F(static_cast<FLOAT>(x), static_cast<FLOAT>(num_grids_y_ * GRID_SIZE)),
            grid_line_brush_,
            1.5f
        );
    }
    for (int y = 0; y < num_grids_y_ * GRID_SIZE + 1; y += GRID_SIZE) {
        d2dRenderTarget->DrawLine(
            D2D1::Point2F(0.0f, static_cast<FLOAT>(y)),
            D2D1::Point2F(static_cast<FLOAT>(num_grids_x_ * GRID_SIZE), static_cast<FLOAT>(y)),
            grid_line_brush_,
            1.5f
        );
    }
    for (int x = 0; x < num_grids_x_; x++) {
        for (int y = 0; y < num_grids_y_; y++) {
            int note = GridLocToMidiNote(x, y);
            D2D1_ELLIPSE ellipse = D2D1::Ellipse(
                D2D1::Point2F(x * GRID_SIZE + GRID_SIZE / 2.f, y * GRID_SIZE + GRID_SIZE / 2.f),
                GRID_SIZE / 5.f,
                GRID_SIZE / 5.f
            );
            if (false) { // note == 64) {
                d2dRenderTarget->FillEllipse(ellipse, highlight_brush_);
            }
            else if (note % 12 == 0) {
                d2dRenderTarget->FillEllipse(ellipse, c_note_brush_);
            }
            else if (((note % 12) == 2) || ((note % 12) == 4) || ((note % 12) == 5) ||
                ((note % 12) == 7) || ((note % 12) == 9) || ((note % 12) == 11)) {
                d2dRenderTarget->FillEllipse(ellipse, note_brush_);

            }
        }
    }
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
    int channel = (id % 10) + 1;
    grid_pointers_[id].channel(channel);
    int midi_pressure = RectToMidiPressure(rect);
    grid_pointers_[id].modulation_z(midi_pressure);
    grid_pointers_[id].modulation_x(-1);
    grid_pointers_[id].modulation_y(-1);
    if (note >= 0) {
        midi_device_->noteOn(channel, note, midi_pressure);

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
    auto& p = grid_pointers_[id];
    p.update(rect, point, pressure);
    int channel = grid_pointers_[id].channel();
    POINT change = p.pointChange();
    int mod_pitch = PointChangeToMidiPitch(change);
    if (mod_pitch != grid_pointers_[id].modulation_x()) {
        // FIXME - maybe rate limit further?
        grid_pointers_[id].modulation_x(mod_pitch);
        midi_device_->pitchBend(channel, mod_pitch);
    }
    int mod_modulation = PointChangeToMidiModulation(change);
    if (mod_modulation != grid_pointers_[id].modulation_y()) {
        // FIXME - maybe rate limit further?
        grid_pointers_[id].modulation_x(mod_modulation);
        midi_device_->controlChange(channel, 1, mod_modulation);
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
    midi_device_->controlChange(channel, 1, 0);
}

int GridStrument::PointToMidiNote(POINT point)
{
    if (point.x > num_grids_x_* GRID_SIZE) {
        return -1;
    }
    if (point.y > num_grids_y_* GRID_SIZE) {
        return -1;
    }
    int x = point.x / GRID_SIZE;
    int y = point.y / GRID_SIZE;
    int note = GridLocToMidiNote(x, y);
    return note;
}

int GridStrument::GridLocToMidiNote(int x, int y)
{
    // Y-invert so up is higher note
    // OLD: put middle C, 64 in the center
    // NEW: put 55 or guitar's 4th string open G on left in middle
    int center_x = num_grids_x_ / 2;
    int center_y = num_grids_y_ / 2;
    // int offset = 64 - (center_x + (num_grids_y_ - 1 - center_y) * 5);
    int offset = 55 - (0 + (num_grids_y_ - 1 - center_y) * 5);
    // might consider guitar-type string change at 59 instead of 60?
    // for the B-string (rather than C) offset
    if (pref_guitar_mode_) {
        if (num_grids_y_ - 1 - y > center_y) {
            offset -= 1;
        }
    }
    int note = offset + x + (num_grids_y_ - 1 - y) * 5;
    if (note > 127) {
        note = 127;
    }
    return note;
}

int GridStrument::RectToMidiPressure(RECT rect)
{
    int x = (rect.right - rect.left);
    int y = (rect.bottom - rect.top);
    int area = x * y;
    float ratio = static_cast<float>(area) / (GRID_SIZE / 2 * GRID_SIZE / 2);
    int pressure = static_cast<int>(ratio * 70);
    if (pressure > 127) {
        pressure = 127;
    }
    return pressure;
}

int GridStrument::PointChangeToMidiPitch(POINT delta)
{
    int dx = delta.x;
    float ratio = static_cast<float>(dx) / GRID_SIZE;
    int pitch = 0x2000 + static_cast<int>(0x2000 * (dx / (12.0f * GRID_SIZE)));
    if (pitch > 0x3fff) {
        pitch = 0x3fff;
    }
    else if (pitch < 0) {
        pitch = 0;
    }
    return pitch;
}

int GridStrument::PointChangeToMidiModulation(POINT delta)
{
    int dy = abs(delta.y);
    float ratio = static_cast<float>(dy) / GRID_SIZE;
    int modulation = static_cast<int>(0x7f * (dy / (1.0f * GRID_SIZE)));
    if (modulation > 0x7f) {
        modulation = 0x7f;
    }
    else if (modulation < 0) {
        modulation = 0;
    }
    return modulation;
}