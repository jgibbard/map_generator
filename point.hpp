#pragma once

struct Point {
    double lng;
    double lat;
    friend bool operator< (const Point& a, const Point& b);
};

bool operator< (const Point& a, const Point& b) {
    if (a.lng < b.lng) {
        return true;
    } else if (a.lng == b.lng && a.lat < b.lat) {
        return true;
    } else {
        return false;
    }
}