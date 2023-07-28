#pragma once

struct Point {
    double x;
    double y;
    friend bool operator< (const Point& a, const Point& b);
    inline void shift(double x_shift, double y_shift) {
        x += x_shift;
        y += y_shift;
    }
    inline void scale(double x_scale, double y_scale) {
        x *= x_scale;
        y *= y_scale;
    }
};

bool operator< (const Point& a, const Point& b) {
    if (a.x < b.x) {
        return true;
    } else if (a.x == b.x && a.y < b.y) {
        return true;
    } else {
        return false;
    }
}