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

GridPointer::GridPointer()
{
    id_ = -1;
    // FIXME initialize all
}

GridPointer::GridPointer(int id, RECT rect, POINT point, int pressure) : 
    id_(id), rect_(rect), point_(point), pressure_(pressure)
{
    starting_point_ = point;
}

void GridPointer::update(RECT rect, POINT point, int pressure)
{
    rect_ = rect;
    point_ = point;
    pressure_ = pressure;
}

POINT GridPointer::pointChange()
{
    POINT p;
    p.x = point_.x - starting_point_.x;
    p.y = point_.y - starting_point_.y;
    return p;
}
