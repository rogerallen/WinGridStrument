#include "GridStrument.h"
#include "GridUtils.h"
#include <cassert>

// type which is both an integer and an array of characters:
typedef union midimessage { unsigned long word; unsigned char data[4]; } MidiMessage;


GridStrument::GridStrument(HMIDIOUT midiDevice)
{
    size_ = D2D1::SizeU(0, 0);
    midi_device_ = midiDevice;
}

void GridStrument::Resize(D2D1_SIZE_U size)
{
    size_ = size;
}

void GridStrument::Draw(ID2D1HwndRenderTarget* d2dRenderTarget)
{

    d2dRenderTarget->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

    for (auto p : grid_pointers_) {
        ID2D1SolidColorBrush* pBrush;
        float pressure = p.second.pressure() / 512.0f;
        const D2D1_COLOR_F color = D2D1::ColorF(pressure, 0.0f, pressure);
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
    // test midi
    MidiMessage message;
    message.data[0] = 0x90;  // MIDI note-on message (requires to data bytes)
    message.data[1] = 60;    // MIDI note-on message: Key number (60 = middle C)
    message.data[2] = 100;   // MIDI note-on message: Key velocity (100 = loud)
    message.data[3] = 0;     // Unused parameter
    MMRESULT rc = midiOutShortMsg(midi_device_, message.word);
    if (rc != MMSYSERR_NOERROR) {
        printf("Warning: MIDI Output is not open.\n");
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

    // test midi
    MidiMessage message;
    message.data[0] = 0x90;  // MIDI note-on message (requires to data bytes)
    message.data[1] = 60;    // MIDI note-on message: Key number (60 = middle C)
    message.data[2] = 0;     // MIDI note-on message: Key velocity (0 = OFF)
    message.data[3] = 0;     // Unused parameter
    MMRESULT rc = midiOutShortMsg(midi_device_, message.word);
    if (rc != MMSYSERR_NOERROR) {
        printf("Warning: MIDI Output is not open.\n");
    }
}
