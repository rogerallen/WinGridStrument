#pragma once
#include <wtypes.h>
class GridPointer
{
    int   _id;
    RECT  _rect;
    POINT _point;
    int   _pressure;
public:
    GridPointer();
    GridPointer(int id, RECT rect, POINT point, int pressure);
    void update(RECT rect, POINT point, int pressure);
    int id() { return _id; };
    RECT rect() { return _rect; };
    POINT point() { return _point; };
    int pressure() { return _pressure; };
};

