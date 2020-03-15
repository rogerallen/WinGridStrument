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
#include <wtypes.h>
class GridPointer
{
    // data from OS
    int   id_;             // unique ID for each touch on screen
    RECT  rect_;           // size of finger touch area, used for pressure
    POINT point_;          // center x,y of touch
    int   pressure_;       // unfortunately, a fixed value and not useful
    POINT starting_point_; // keep track of initial x,y location
    // higher-level associated data
    int note_;             // midi note of initial x,y
    int channel_;          // midi channel selected for this pointer  
    int modulation_x_;     // midi modulation in +/- X direction
    int modulation_y_;     // midi modulation in +/- Y direction
    int modulation_z_;     // midi modulation in +/- Z direction (pressure)
public:
    GridPointer();
    GridPointer(int id, RECT rect, POINT point, int pressure);
    void update(RECT rect, POINT point, int pressure);
    POINT pointChange();
    // getters
    int id() { return id_; };
    RECT rect() { return rect_; };
    POINT point() { return point_; };
    int pressure() { return pressure_; };
    // getters/setters
    void note(int note) { note_ = note; }
    int note() { return note_; }
    void channel(int channel) { channel_ = channel; }
    int channel() { return channel_; }
    void modulationX(int modulation_x) { modulation_x_ = modulation_x; }
    int modulationX() { return modulation_x_; }
    void modulationY(int modulation_y) { modulation_y_ = modulation_y; }
    int modulationY() { return modulation_y_; }
    void modulationZ(int modulation_z) { modulation_z_ = modulation_z; }
    int modulationZ() { return modulation_z_; }
};

