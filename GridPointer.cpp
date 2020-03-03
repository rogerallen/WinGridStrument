#include "GridPointer.h"

GridPointer::GridPointer()
{
    id_ = -1;
    // FIXME initialize all
}

GridPointer::GridPointer(int id, RECT rect, POINT point, int pressure) : 
    id_(id), rect_(rect), point_(point), pressure_(pressure)
{
}

void GridPointer::update(RECT rect, POINT point, int pressure)
{
    rect_ = rect;
    point_ = point;
    pressure_ = pressure;
}
