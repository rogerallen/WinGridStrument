#pragma once
#include <wtypes.h>
class GridPointer
{
    int   id_;
    RECT  rect_;
    POINT point_;
    int   pressure_;
public:
    GridPointer();
    GridPointer(int id, RECT rect, POINT point, int pressure);
    void update(RECT rect, POINT point, int pressure);
    int id() { return id_; };
    RECT rect() { return rect_; };
    POINT point() { return point_; };
    int pressure() { return pressure_; };
};

