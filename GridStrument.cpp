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
#include <vector>

// ======================================================================
// forward decl for "public" methods for dealing with hex grids
// FIXME - refactor this to be more object-oriented instead of functional
void hexGetNumGrids(int screen_width, int screen_height, int grid_size, int& num_grids_x, int& num_grids_y);
void hexDrawCell(ID2D1HwndRenderTarget* d2dRenderTarget, ID2D1SolidColorBrush* brush, int grid_size, int num_grids_x, int num_grids_y, int x, int y);
int hexGridLocToMidiNote(int num_grids_y, int x, int y);
D2D1_POINT_2F hexGridLocToPoint(int grid_size, int x, int y);
void pointToHexGridLoc(POINT point, int grid_size, int& x, int& y);

// ======================================================================
// Main constructor, set defaults and midi output device
GridStrument::GridStrument(HMIDIOUT midiDevice)
{
    // initial preferences.  These get updated by WinGridStrument code
    pref_guitar_mode_ = true;
    pref_pitch_bend_range_ = 12;
    pref_modulation_controller_ = 1;
    pref_midi_channel_min_ = 0;
    pref_midi_channel_max_ = 10;
    pref_grid_size_ = 90;
    pref_channel_per_row_mode_ = false;
    pref_pitch_bend_mask_ = 0x3fff;
    pref_hex_grid_mode_ = true;
    pref_play_midi_ = false;
    pref_play_soundfont_ = false; 
    pref_soundfont_path_ = "";

    size_ = D2D1::SizeU(0, 0);
    num_grids_x_ = num_grids_y_ = 0;

    grid_synth_ = new GridSynth();

    midi_device_ = new GridMidi(midiDevice, grid_synth_);
    midi_channel_ = pref_midi_channel_min_;

}

GridStrument::~GridStrument()
{
    free(midi_device_);
    free(grid_synth_);
}

// ======================================================================
// update the midi output device
//
void GridStrument::midiDevice(HMIDIOUT midiDevice) 
{
    free(midi_device_);
    midi_device_ = new GridMidi(midiDevice, grid_synth_);
}

// ======================================================================
// resize and adjust the num_grids
//
void GridStrument::resize(D2D1_SIZE_U size)
{
    size_ = size;
    if (!pref_hex_grid_mode_) {
        num_grids_x_ = (int)(size_.width / pref_grid_size_);
        num_grids_y_ = (int)(size_.height / pref_grid_size_);
    }
    else {
        hexGetNumGrids((int)size_.width, (int)size_.height, pref_grid_size_ / 2, num_grids_x_, num_grids_y_);
    }

#ifndef NDEBUG
    std::wcout << "screen width = " << size.width << ", height = " << size.height << std::endl;
    std::wcout << "screen columns = " << num_grids_x_ << ", rows = " << num_grids_y_ << std::endl;
    std::wcout << "min note = " << gridLocToMidiNote(0, num_grids_y_ - 1) << std::endl;
    std::wcout << "max note = " << gridLocToMidiNote(num_grids_x_ - 1, 0) << std::endl;
#endif // !NDEBUG
}

// ======================================================================
// main drawing routine.  create brushes if necessary & then draw the
// grid, notes, etc.
//
void GridStrument::draw(ID2D1HwndRenderTarget* d2dRenderTarget, IDWriteTextFormat* dwriteTextFormat)
{
    if (!brushes_.initialized_) {
        brushes_.init(d2dRenderTarget);
    }
    d2dRenderTarget->Clear(brushes_.color_theme_.clearColor());
    if (pref_guitar_mode_) {
        drawGuitar(d2dRenderTarget);
    }
    drawGrid(d2dRenderTarget);
    drawDots(d2dRenderTarget);
    drawText(d2dRenderTarget, dwriteTextFormat);
    drawPointers(d2dRenderTarget);
}

// ======================================================================
// draw the touch rectangles for each finger
//
void GridStrument::drawPointers(ID2D1HwndRenderTarget* d2dRenderTarget)
{
    // FIXME add pointer brush to brushes_, it isn't dynamic any longer.
    for (auto p : grid_pointers_) {
        ID2D1SolidColorBrush* pBrush;
        HRESULT hr = d2dRenderTarget->CreateSolidColorBrush(brushes_.color_theme_.touchColor(), &pBrush);
        if (SUCCEEDED(hr)) {
            RECT rc = p.second.rect();
            D2D1_RECT_F rcf = D2D1::RectF((float)rc.left, (float)rc.top, (float)rc.right, (float)rc.bottom);
            d2dRenderTarget->FillRectangle(&rcf, pBrush);
            SafeRelease(&pBrush);
        }
    }
}

// ======================================================================
// draw the dots for each note. C gets a special color.
// Notes that are currently pressed also get a special color.
//
void GridStrument::drawDots(ID2D1HwndRenderTarget* d2dRenderTarget)
{
    std::set<int> active_notes;
    for (auto p : grid_pointers_) {
        active_notes.insert(p.second.note());
    }
    for (int x = 0; x < num_grids_x_; x++) {
        for (int y = 0; y < num_grids_y_; y++) {
            int note;
            D2D1_POINT_2F center;
            if (!pref_hex_grid_mode_) {
                note = gridLocToMidiNote(x, y);
                center = D2D1::Point2F(x * pref_grid_size_ + pref_grid_size_ / 2.f, y * pref_grid_size_ + pref_grid_size_ / 2.f);
            }
            else {
                note = hexGridLocToMidiNote(num_grids_y_, x, y);
                center = hexGridLocToPoint(pref_grid_size_ / 2, x, y);
            }
            D2D1_ELLIPSE ellipse = D2D1::Ellipse(
                center,
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

// ======================================================================
// draw the text for each note. 
//
void GridStrument::drawText(ID2D1HwndRenderTarget* d2dRenderTarget, IDWriteTextFormat* dwriteTextFormat)
{
    static const WCHAR note_names[][3] = {
        // Just in case sharp: ♯ flat: ♭ and a natural: ♮
        L"C ", L"C♯", L"D ", L"D♯", L"E ", L"F ", L"F♯", L"G ", L"G♯", L"A ", L"A♯", L"B "
        //L"C ", L"D♭", L"D ", L"E♭", L"E ", L"F ", L"G♭", L"G ", L"A♭", L"A ", L"B♭", L"B "
    };
    for (int x = 0; x < num_grids_x_; x++) {
        for (int y = 0; y < num_grids_y_; y++) {
            float left, top, right, bottom;
            int note_index;
            if (!pref_hex_grid_mode_) {
                left = 1.0f * x * pref_grid_size_ + 5;
                // bottom of square
                // float top = 1.0f * (y + 1) * pref_grid_size_ - 20;
                // top of square
                top = 1.0f * y * pref_grid_size_ + 5;
                right = left + 100;
                bottom = top + 100;
                int note = gridLocToMidiNote(x, y);
                note_index = note % 12;
            }
            else {
                //int mid_y = num_grids_y_ - 1 - num_grids_y_ / 2;
                //assert(55 == hexGridLocToMidiNote(num_grids_y_, 0, mid_y));
                //assert(55+7 == hexGridLocToMidiNote(num_grids_y_, 0, mid_y - 1));
                D2D1_POINT_2F center = hexGridLocToPoint(pref_grid_size_ / 2, x, y);
                left = center.x - pref_grid_size_ / 2 + (float)(sqrt(3)*15);
                top = center.y - pref_grid_size_ / 2 + 10;
                right = left + 100;
                bottom = top + 100;
                int note = hexGridLocToMidiNote(num_grids_y_, x, y);
                note_index = note % 12;
            }
            d2dRenderTarget->DrawText(
                note_names[note_index],
                2,
                dwriteTextFormat,
                D2D1::RectF(left, top, right, bottom),
                brushes_.grid_line_
                );
        }
    }

}

// ======================================================================
// draw a background that highlights the six "guitar string" rows when
// we are in guitar_mode_.
//
void GridStrument::drawGuitar(ID2D1HwndRenderTarget* d2dRenderTarget)
{
    if (pref_hex_grid_mode_)
        return;

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

// ======================================================================
// draw the lines making up the grid
//
void GridStrument::drawGrid(ID2D1HwndRenderTarget* d2dRenderTarget)
{
    if (!pref_hex_grid_mode_) {
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
    else {
        for (int x = 0; x < num_grids_x_; x++) {
            for (int y = 0; y < num_grids_y_; y++) {
                hexDrawCell(d2dRenderTarget, brushes_.grid_line_, pref_grid_size_ / 2, num_grids_x_, num_grids_y_, x, y);
            }
        }
    }
}

// ======================================================================
// handle the pointerDown event.  We get the ID, touch rectangle, center point
// and while we do get pressure, it isn't useful and we use the rectangle
// area for a "pressure"-like value to store as a modulationZ value.
//
// Add a GridPointer to the grid_pointer_ dictionary & index with the ID.
// set all midi values for note, channel & modulationX/Y/Z
//
// Finally, if a note is struck, send it to the midi device.
//
void GridStrument::pointerDown(int id, RECT rect, POINT point, int pressure)
{
#ifndef NDEBUG
    for (auto pair : grid_pointers_) {
        assert(pair.first != id);
    }
#endif  
    grid_pointers_.emplace(id, GridPointer(id, rect, point, pressure));
    int note = pointToMidiNote(point);
    grid_pointers_[id].note(note);
    // assume default midi channel mode
    int channel = midi_channel_;
    nextMidiChannel();
    // override in the case of per-row-mode
    if (pref_channel_per_row_mode_) {
        // use as many channels as you can, given min/max range
        int row = pointToGridRow(point);
        channel = row % (pref_midi_channel_max_ + 1 - pref_midi_channel_min_);
        channel += pref_midi_channel_min_;
        midi_channel_ = channel;
    }
    grid_pointers_[id].channel(channel);
    int midi_pressure = rectToMidiPressure(rect);
    grid_pointers_[id].modulationZ(midi_pressure);
    grid_pointers_[id].modulationX(0);
    grid_pointers_[id].modulationY(0);
    if (note >= 0) {
        midi_device_->noteOn(channel, note, midi_pressure);
    }
}

// ======================================================================
// default way to choose next midi channel.
//
void GridStrument::nextMidiChannel() 
{
    midi_channel_++;
    if (midi_channel_ > pref_midi_channel_max_) {
        midi_channel_ = pref_midi_channel_min_;
    }
}

// ======================================================================
// Handle pointer update event.  Update the GridPointer with new 
// modulationX/Y/Z values and send them to the midi device.
//
void GridStrument::pointerUpdate(int id, RECT rect, POINT point, int pressure)
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
    int mod_pitch = pointChangeToPitchBend(change);
    if (mod_pitch != cur_ptr.modulationX()) {
        cur_ptr.modulationX(mod_pitch);
        midi_device_->pitchBend(channel, mod_pitch);
    }
    int mod_modulation = pointChangeToMidiModulation(change);
    if (mod_modulation != cur_ptr.modulationY()) {
        cur_ptr.modulationY(mod_modulation);
        midi_device_->controlChange(channel, pref_modulation_controller_, mod_modulation);
    }
    int midi_pressure = rectToMidiPressure(rect);
    if (midi_pressure != cur_ptr.modulationZ()) {
        cur_ptr.modulationZ(midi_pressure);
        int note = cur_ptr.note();
        midi_device_->polyKeyPressure(channel, note, midi_pressure);
    }
}

// ======================================================================
// handle when the finger touch goes away.  Shut everything down and
// remove it from the grid_pointers_ dictionary.
//
void GridStrument::pointerUp(int id)
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

// ======================================================================
// convert window point to grid x.  -1 if not in the grid
//
int GridStrument::pointToGridColumn(POINT point) 
{
    if (point.x > num_grids_x_ * pref_grid_size_) {
        return -1;
    }
    int x = point.x / pref_grid_size_;
    return x;
}

// ======================================================================
// convert window point to grid y. -1 if not in the grid
//
int GridStrument::pointToGridRow(POINT point) 
{
    if (point.y > num_grids_y_ * pref_grid_size_) {
        return -1;
    }
    int y = point.y / pref_grid_size_;
    return y;
}

// ======================================================================
// convert window point to a midi note value.  return -1 if not in the grid
int GridStrument::pointToMidiNote(POINT point)
{
    int note;
    if (!pref_hex_grid_mode_) {
        int x = pointToGridColumn(point);
        if (x < 0) {
            return -1;
        }
        int y = pointToGridRow(point);
        if (y < 0) {
            return -1;
        }
        note = gridLocToMidiNote(x, y);
    }
    else {
        int x, y;
        pointToHexGridLoc(point, pref_grid_size_/2, x, y); // updates x,y
        note = hexGridLocToMidiNote(num_grids_y_, x, y);
    }
    return note;
}

// ======================================================================
// map grid location to a midi note.  Handle Y-inversion so higher notes
// are towards the top.  Also handle guitar_mode_
// values are clamped to the range [0,127]
//
int GridStrument::gridLocToMidiNote(int x, int y)
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

// ======================================================================
// A finger's pressure/modulationZ is determined by the size of the
// area touched.
//
int GridStrument::rectToMidiPressure(RECT rect)
{
    int x = (rect.right - rect.left);
    int y = (rect.bottom - rect.top);
    int area = x * y;
    float ratio = static_cast<float>(area) / (pref_grid_size_ / 2 * pref_grid_size_ / 2);
    ratio = sqrtf(ratio) - 0.25f;  // linearize it
    int pressure = static_cast<int>(ratio * 100);
    if (pressure > 127) {
        pressure = 127;
    }
    return pressure;
}

// ======================================================================
// Look at how the deltaX value has changed and convert to a
// pitch bend range value.  This is centered at 0x2000 and ranges from
// 0 to 0x3fff.  So, the note can bend down and up.  The pitch_bend_range
// tells you how many grids you can go before it hits full range.
//
// Use mask to remove lower bits.  This can help reduce "noisy" events.
// 
int GridStrument::pointChangeToPitchBend(POINT delta)
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

// ======================================================================
// Look at how the deltaY value has changed and convert that to a 
// modulation value.  This adjusts from 0-0x7f where 0 is at the center
// and moving either up or down adjusts the value towards 0x7f.
// One grid unit up or down will hit full range.
//
int GridStrument::pointChangeToMidiModulation(POINT delta)
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

// ======================================================================
// hex notes
// https://www.redblobgames.com/grids/hexagons/
// https://www.redblobgames.com/grids/hexagons/implementation.html
// q (flat top, instead of r pointy top) orientation matches
//    https://en.wikipedia.org/wiki/Harmonic_table_note_layout
// odd-q puts 0,0 in upper left corner.  let's use that
// 

// Code below based on 
// Generated code -- CC0 -- No Rights Reserved -- http://www.redblobgames.com/grids/hexagons/
//

struct Point
{
    const double x;
    const double y;
    Point(double x_, double y_) : x(x_), y(y_) {}
};

// These are cube coords, our native is offset coords (see below)
// we need to convert from Offset to Hex 
struct Hex
{
    const int q;
    const int r;
    const int s;
    Hex(int q_, int r_, int s_) : q(q_), r(r_), s(s_) {
        if (q + r + s != 0) throw "q + r + s must be 0";
    }
};

struct FractionalHex
{
    const double q;
    const double r;
    const double s;
    FractionalHex(double q_, double r_, double s_) : q(q_), r(r_), s(s_) {
        if (round(q + r + s) != 0) throw "q + r + s must be 0";
    }
};

struct OffsetCoord
{
    const int col;
    const int row;
    OffsetCoord(int col_, int row_) : col(col_), row(row_) {}
};

struct FractionalOffsetCoord
{
    const double col;
    const double row;
    FractionalOffsetCoord(double col_, double row_) : col(col_), row(row_) {}
};

struct Orientation
{
    const double f0;
    const double f1;
    const double f2;
    const double f3;
    const double b0;
    const double b1;
    const double b2;
    const double b3;
    const double start_angle;
    Orientation(double f0_, double f1_, double f2_, double f3_, double b0_, double b1_, double b2_, double b3_, double start_angle_) : f0(f0_), f1(f1_), f2(f2_), f3(f3_), b0(b0_), b1(b1_), b2(b2_), b3(b3_), start_angle(start_angle_) {}
};

struct Layout
{
    const Orientation orientation;
    const Point size;
    const Point origin;
    Layout(Orientation orientation_, Point size_, Point origin_) : orientation(orientation_), size(size_), origin(origin_) {}
};

const Orientation flat_orientation = Orientation(3.0 / 2.0, 0.0, sqrt(3.0) / 2.0, sqrt(3.0), 2.0 / 3.0, 0.0, -1.0 / 3.0, sqrt(3.0) / 3.0, 0.0);

Point hex_to_pixel(Layout layout, Hex h)
{
    Orientation M = layout.orientation;
    Point size = layout.size;
    Point origin = layout.origin;
    double x = (M.f0 * h.q + M.f1 * h.r) * size.x;
    double y = (M.f2 * h.q + M.f3 * h.r) * size.y;
    return Point(x + origin.x, y + origin.y);
}

FractionalHex pixel_to_hex(Layout layout, Point p)
{
    Orientation M = layout.orientation;
    Point size = layout.size;
    Point origin = layout.origin;
    Point pt = Point((p.x - origin.x) / size.x, (p.y - origin.y) / size.y);
    double q = M.b0 * pt.x + M.b1 * pt.y;
    double r = M.b2 * pt.x + M.b3 * pt.y;
    return FractionalHex(q, r, -q - r);
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Point hex_corner_offset(Layout layout, int corner) {
    Point size = layout.size;
    double angle = 2.0 * M_PI *
        (layout.orientation.start_angle + corner) / 6;
    return Point(size.x * cos(angle), size.y * sin(angle));
}

std::vector<Point> polygon_corners(Layout layout, Hex h) {
    std::vector<Point> corners = {};
    Point center = hex_to_pixel(layout, h);
    for (int i = 0; i < 6; i++) {
        Point offset = hex_corner_offset(layout, i);
        corners.push_back(Point(center.x + offset.x,
            center.y + offset.y));
    }
    return corners;
}

const double EVEN = 1;
const double ODD = -1;
OffsetCoord qoffset_from_cube(int offset, Hex h)
{
    int col = h.q;
    int row = h.r + int((h.q + offset * (h.q & 1)) / 2);
    if (offset != EVEN && offset != ODD)
    {
        throw "offset must be EVEN (+1) or ODD (-1)";
    }
    return OffsetCoord(col, row);
}
FractionalOffsetCoord qoffset_from_cube(double offset, FractionalHex h)
{
    double col = h.q;
    double odd_q = static_cast<int>(h.q) & 1 ? 1 : 0;
    double row = h.r + int((h.q + offset * odd_q) / 2);
    if (offset != EVEN && offset != ODD)
    {
        throw "offset must be EVEN (+1) or ODD (-1)";
    }
    return FractionalOffsetCoord(col, row);
}

Hex qoffset_to_cube(int offset, OffsetCoord h)
{
    int q = h.col;
    int r = h.row - int((h.col + offset * (h.col & 1)) / 2);
    int s = -q - r;
    if (offset != EVEN && offset != ODD)
    {
        throw "offset must be EVEN (+1) or ODD (-1)";
    }
    return Hex(q, r, s);
}

Hex hex_round(FractionalHex h)
{
    int qi = int(round(h.q));
    int ri = int(round(h.r));
    int si = int(round(h.s));
    double q_diff = abs(qi - h.q);
    double r_diff = abs(ri - h.r);
    double s_diff = abs(si - h.s);
    if (q_diff > r_diff && q_diff > s_diff)
    {
        qi = -ri - si;
    }
    else
        if (r_diff > s_diff)
        {
            ri = -qi - si;
        }
        else
        {
            si = -qi - ri;
        }
    return Hex(qi, ri, si);
}

// ======================================================================
// "public" methods for dealing with hex grids
// FIXME - refactor this to be more object-oriented instead of functional
void hexGetNumGrids(int screen_width, int screen_height, int grid_size, int& num_grids_x, int& num_grids_y)
{
    Layout l = Layout(flat_orientation, Point(grid_size, grid_size), Point(grid_size, sqrt(3) * grid_size / 2));
    FractionalHex fh = pixel_to_hex(l, Point(screen_width, screen_height));
    FractionalOffsetCoord oc = qoffset_from_cube(ODD, fh);
    num_grids_x = static_cast<int>(oc.col);
    num_grids_y = static_cast<int>(oc.row);
}

void hexDrawCell(ID2D1HwndRenderTarget* d2dRenderTarget, ID2D1SolidColorBrush* brush, int grid_size, int num_grids_x, int num_grids_y, int x, int y)
{
    OffsetCoord oc = OffsetCoord(x, y);
    Hex h = qoffset_to_cube((int)ODD, oc);
    Layout l = Layout(flat_orientation, Point(grid_size, grid_size), Point(grid_size, sqrt(3) * grid_size / 2));
    std::vector<Point> points = polygon_corners(l, h);
    // drawing all 6 lines results in lots of overdraw, so we do something a bit more complicated
    // to avoid the overdraw
    for (int i = 3; i < 6; i++) {
        float x0 = static_cast<float>(points[i].x);
        float y0 = static_cast<float>(points[i].y);
        float x1 = static_cast<float>(points[(i + 1) % 6].x);
        float y1 = static_cast<float>(points[(i + 1) % 6].y);
        d2dRenderTarget->DrawLine(D2D1::Point2F(x0, y0), D2D1::Point2F(x1, y1), brush, 1.5f);
    }
    if (x == 0) {
        int i = 2;
        float x0 = static_cast<float>(points[i].x);
        float y0 = static_cast<float>(points[i].y);
        float x1 = static_cast<float>(points[(i + 1) % 6].x);
        float y1 = static_cast<float>(points[(i + 1) % 6].y);
        d2dRenderTarget->DrawLine(D2D1::Point2F(x0, y0), D2D1::Point2F(x1, y1), brush, 1.5f);
    }
    else if (x == num_grids_x - 1) {
        int i = 0;
        float x0 = static_cast<float>(points[i].x);
        float y0 = static_cast<float>(points[i].y);
        float x1 = static_cast<float>(points[(i + 1) % 6].x);
        float y1 = static_cast<float>(points[(i + 1) % 6].y);
        d2dRenderTarget->DrawLine(D2D1::Point2F(x0, y0), D2D1::Point2F(x1, y1), brush, 1.5f);
    }
    if (y == num_grids_y - 1) {
        if ((x & 1) == 0) { // even
            int i = 1;
            float x0 = static_cast<float>(points[i].x);
            float y0 = static_cast<float>(points[i].y);
            float x1 = static_cast<float>(points[(i + 1) % 6].x);
            float y1 = static_cast<float>(points[(i + 1) % 6].y);
            d2dRenderTarget->DrawLine(D2D1::Point2F(x0, y0), D2D1::Point2F(x1, y1), brush, 1.5f);
        }
        else {
            for (int i = 0; i < 3; i++) {
                float x0 = static_cast<float>(points[i].x);
                float y0 = static_cast<float>(points[i].y);
                float x1 = static_cast<float>(points[(i + 1) % 6].x);
                float y1 = static_cast<float>(points[(i + 1) % 6].y);
                d2dRenderTarget->DrawLine(D2D1::Point2F(x0, y0), D2D1::Point2F(x1, y1), brush, 1.5f);
            }
        }
    }
}

int hexGridLocToMidiNote(int num_grids_y, int x, int y)
{
    // Y-Delta is like the X row size in normal array index math
    // It is fixed at 7 to match going up by musical *fifths*
    int Y_DELTA = 7;
    // Y-invert so up is higher note.  Grid Y ranges from 0..(num-1)
    y = num_grids_y - 1 - y;
    int center_y = num_grids_y / 2;
    int note;
    if ((x & 1) == 0) {
        // EVEN: put 55 or guitar's 4th string open G on left in middle
        int offset = 55 - (0 + center_y * Y_DELTA);
        x = x / 2;
        note = offset + x + y * Y_DELTA;
    }
    else {
        // ODD: put E-below-G as left-middle offset for odd strings
        // (so E + minor third = G)
        // g f# f e = 55 54 53 52
        int offset = 52 - (0 + center_y * Y_DELTA);
        x = (x - 1) / 2;
        note = offset + x + y * Y_DELTA;
    }
    note = std::clamp(note, 0, 127);
    return note;

}

D2D1_POINT_2F hexGridLocToPoint(int grid_size, int x, int y)
{
    OffsetCoord oc = OffsetCoord(x, y);
    Hex h = qoffset_to_cube((int)ODD, oc);
    Layout l = Layout(flat_orientation, Point(grid_size, grid_size), Point(grid_size, sqrt(3) * grid_size / 2));
    Point center = hex_to_pixel(l, h);
    return  D2D1::Point2F(static_cast<float>(center.x), static_cast<float>(center.y));
}

void pointToHexGridLoc(POINT point, int grid_size, int& x, int& y)
{
    Layout l = Layout(flat_orientation, Point(grid_size, grid_size), Point(grid_size, sqrt(3) * grid_size / 2));
    FractionalHex fh = pixel_to_hex(l, Point(point.x, point.y));
    // If you use offset coordinates, use return cube_to_{odd,even}{r,q}(cube_round(Cube(q, -q-r, r))).
    Hex h = hex_round(fh);
    OffsetCoord oc = qoffset_from_cube((int)ODD, h);
    x = static_cast<int>(oc.col);
    y = static_cast<int>(oc.row);
}
