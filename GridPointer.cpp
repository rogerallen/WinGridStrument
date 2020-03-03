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
