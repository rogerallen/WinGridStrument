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
#include <wtypes.h>
class GridPointer
{
    int   id_;
    RECT  rect_;
    POINT point_;
    int   pressure_;
    POINT starting_point_;
    // higher-level associated data
    int note_;
    int channel_;
    int modulation_x_;
    int modulation_y_;
    int modulation_z_;
public:
    GridPointer();
    GridPointer(int id, RECT rect, POINT point, int pressure);
    void update(RECT rect, POINT point, int pressure);
    int id() { return id_; };
    RECT rect() { return rect_; };
    POINT point() { return point_; };
    int pressure() { return pressure_; };
    POINT pointChange();
    void note(int note) { note_ = note; }
    int note() { return note_; }
    void channel(int channel) { channel_ = channel; }
    int channel() { return channel_; }
    void modulation_x(int modulation_x) { modulation_x_ = modulation_x; }
    int modulation_x() { return modulation_x_; }
    void modulation_y(int modulation_y) { modulation_y_ = modulation_y; }
    int modulation_y() { return modulation_y_; }
    void modulation_z(int modulation_z) { modulation_z_ = modulation_z; }
    int modulation_z() { return modulation_z_; }
};

