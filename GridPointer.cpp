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
#include "GridPointer.h"

// ======================================================================
// Default contstructor.  FIXME would like to know why I need this.
//
GridPointer::GridPointer()
{
    id_ = -1;
    rect_ = RECT{ 0,0,0,0 };
    point_ = POINT{ 0,0 };
    pressure_ = 0;
    starting_point_ = POINT{ 0,0 };
    note_ = 0;
    channel_ = 0;
    modulation_x_ = modulation_y_ = modulation_z_ = 0;
}

// ======================================================================
// Standard Constructor.  Used when we get a finger down touch event.
//
GridPointer::GridPointer(int id, RECT rect, POINT point, int pressure) :
    id_(id), rect_(rect), point_(point), pressure_(pressure)
{
    starting_point_ = point;
    // setters will update these values
    note_ = 0;
    channel_ = 0;
    modulation_x_ = modulation_y_ = modulation_z_ = 0;
}

// ======================================================================
// update the pointer values on touch update event
// 
void GridPointer::update(RECT rect, POINT point, int pressure)
{
    rect_ = rect;
    point_ = point;
    pressure_ = pressure;
}

// ======================================================================
// return the dx,dy between the current point and starting point.
//
POINT GridPointer::pointChange()
{
    POINT delta;
    delta.x = point_.x - starting_point_.x;
    delta.y = point_.y - starting_point_.y;
    return delta;
}
