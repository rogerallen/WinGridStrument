#include "GridPointer.h"

GridPointer::GridPointer()
{
    _id = -1;
    // FIXME initialize all
}

GridPointer::GridPointer(int id, RECT rect, POINT point, int pressure) : 
    _id(id), _rect(rect), _point(point), _pressure(pressure)
{
}

void GridPointer::update(RECT rect, POINT point, int pressure)
{
    _rect = rect;
    _point = point;
    _pressure = pressure;
}
