#include "GridStrument.h"
#include "GridUtils.h"
#include <cassert>
#include <iostream>

// type which is both an integer and an array of characters:
typedef union midimessage { unsigned long word; unsigned char data[4]; } MidiMessage;

static const int GRID_SIZE = 90;

GridStrument::GridStrument(HMIDIOUT midiDevice)
{
    size_ = D2D1::SizeU(0, 0);
    num_grids_x_ = num_grids_y_ = 0;
    midi_device_ = midiDevice;
    grid_line_brush_ = nullptr;
    c_note_brush_ = nullptr;
    note_brush_ = nullptr;
}

void GridStrument::Resize(D2D1_SIZE_U size)
{
    size_ = size;
    num_grids_x_ = (int)(size_.width / GRID_SIZE);
    num_grids_y_ = (int)(size_.height / GRID_SIZE);

#ifndef NDEBUG
    std::wcout << "screen width = " << size.width << ", height = " << size.height << std::endl;
    std::wcout << "screen x = " << num_grids_x_ << ", y = " << num_grids_y_ << std::endl;
    std::wcout << "max note = " << GridLocToMidiNote(num_grids_x_-1, num_grids_y_-1) << std::endl;
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
            int note = GridLocToMidiNote(x,y);
            D2D1_ELLIPSE ellipse = D2D1::Ellipse(
                D2D1::Point2F(x * GRID_SIZE + GRID_SIZE / 2.f, y * GRID_SIZE + GRID_SIZE / 2.f),
                GRID_SIZE / 5.f,
                GRID_SIZE / 5.f
            );
            if (note % 12 == 0) {
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
    grid_notes_.emplace(id, note);
    int channel = (id % 10) + 1;
    grid_channels_.emplace(id, channel);
    int midi_pressure = RectToMidiPressure(rect);
    grid_mod_pitch_.emplace(id, -1);
    if (note >= 0) {
        MidiMessage message;
        message.data[0] = 0x90 + channel;  // MIDI note-on message (requires to data bytes)
        message.data[1] = note;  // MIDI note-on message: Key number (60 = middle C)
        message.data[2] = midi_pressure;   // MIDI note-on message: Key velocity (100 = loud)
        message.data[3] = 0;     // Unused parameter
        MMRESULT rc = midiOutShortMsg(midi_device_, message.word);
        if (rc != MMSYSERR_NOERROR) {
            printf("Warning: MIDI Output is not open.\n");
        }
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
    int channel = grid_channels_[id];
    POINT change = p.pointChange();
    int mod_pitch = PointChangeToMidiPitch(change);
    if (mod_pitch != grid_mod_pitch_[id]) {
        // FIXME - maybe rate limit further?
        grid_mod_pitch_[id] = mod_pitch;
        MidiMessage message;
        message.data[0] = 0xe0 + channel;  // MIDI Pitch Bend Change
        message.data[1] = mod_pitch & 0x7f;  // low bits
        message.data[2] = (mod_pitch >> 7) & 0x7f; // high bits
        message.data[3] = 0;     // Unused parameter
        MMRESULT rc = midiOutShortMsg(midi_device_, message.word);
        if (rc != MMSYSERR_NOERROR) {
            printf("Warning: MIDI Output is not open.\n");
        }
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
    grid_pointers_.erase(id);
    int note = grid_notes_[id];
    grid_notes_.erase(id);
    int channel = grid_channels_[id];
    grid_channels_.erase(id);
    grid_mod_pitch_.erase(id);
    if (note >= 0) {
        MidiMessage message;
        message.data[0] = 0x90 + channel;  // MIDI note-on message (requires to data bytes)
        message.data[1] = note;  // MIDI note-on message: Key number (60 = middle C)
        message.data[2] = 0;     // MIDI note-on message: Key velocity (0 = OFF)
        message.data[3] = 0;     // Unused parameter
        MMRESULT rc = midiOutShortMsg(midi_device_, message.word);
        if (rc != MMSYSERR_NOERROR) {
            printf("Warning: MIDI Output is not open.\n");
        }
    }
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
    int note = 2*12 + x + y * 7;
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
    float ratio = static_cast<float>(area) / (GRID_SIZE/2 * GRID_SIZE/2);
    int pressure = ratio * 70;
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