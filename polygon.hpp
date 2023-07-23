#pragma once
#include <vector>
#include <algorithm>    // min_element
#include <limits>       // numeric_limits
#include "point.hpp"

struct Polygon {
    std::vector<Point> outer;
    std::vector<std::vector<Point>> inner;
    std::pair<Point, Point> bounding_box;

    bool contains(const Point& p) {
        // If point is within inner boundary, then it is 
        // not within the polygon
        for (auto& inner_boundary : inner) {
            if (contains(inner_boundary, p)) return false;
        }
        // Otherwise, check if it is within the outer boundary
        return contains(outer, p);
    }

    static bool contains(std::vector<Point>::const_iterator start,
                         std::vector<Point>::const_iterator stop, const Point& p) {

        unsigned int count = 0;
        auto it = start;
        while(it != (stop - 1)) {
            if (_intersects(*it, *(it+1), p)) count++;
            it++;
        }
        // If odd number of intersections, then P is within the region
        return count % 2 != 0;
    }

    static bool contains(const std::vector<Point>& points, const Point& p) {

        unsigned int points_length = points.size();
        unsigned int count = 0;
        for (unsigned int i = 0; i <= points_length - 2; i++) {
            if (_intersects(points[i],points[i+1],p)) {
                count++;
            }
        }
        // If odd number of intersections, then P is within the region
        return count % 2 != 0;
    }
    static bool is_clockwise(const std::vector<Point>& points) {
        return is_clockwise(points.begin(), points.end());
    }
    static bool is_clockwise(std::vector<Point>::const_iterator start,
                            std::vector<Point>::const_iterator stop) {
        
        // https://en.wikipedia.org/wiki/Curve_orientation

        // Find the point with lowest lat (if lat is the same, find with lowest lng)
        auto b = std::min_element(start,stop);
        // Get point before and after the lowest point
        // Need to handle wrapping at edges of vector
        // First and last points are always equal so wrap to second to last point
        auto a = (b == start) ? (stop - 2) : b - 1;
        // First and last points are always equal so wrap to second from first point
        auto c = (b == (stop - 1)) ? (start + 1) : b + 1;

        double det = ((b->lng - a->lng) * (c->lat - a->lat)) - ((c->lng - a->lng) * (b->lat - a->lat));

        return det < 0;
    }

    static std::pair<Point, Point> get_bounding_box(std::vector<Point>::const_iterator start,
                                                  std::vector<Point>::const_iterator stop) {

        // Make min the maximum possible value, and max the min possible value
        Point min({std::numeric_limits<double>().max(), std::numeric_limits<double>().max()});
        Point max({std::numeric_limits<double>().min(), std::numeric_limits<double>().min()});

        auto it = start;
        while (it != stop) {
            if (it->lat < min.lat) min.lat = it->lat;
            if (it->lng < min.lng) min.lng = it->lng;
            if (it->lat > max.lat) max.lat = it->lat;
            if (it->lng > max.lng) max.lng = it->lng;
            it++;
        }
        return {min,max};
    }
    static std::pair<Point, Point> get_bounding_box(std::vector<Point>& points) {
        return get_bounding_box(points.begin(), points.end());
    }

private:
    static bool _intersects(const Point& a, const Point& b, const Point& p) {

        double kEpsilon = static_cast<double>(std::numeric_limits<float>().epsilon());
        double kMin = std::numeric_limits<double>().min();
        double kMax = std::numeric_limits<double>().max();

        // Algorithm works when a longitude is less than or equal to b longitude
        // So flip vertices around
        if (a.lng > b.lng) {
            return _intersects(b, a, p);
        }
        // Algorithm only works when p longitude is not the same as a or b.
        // So add a very small value to p longitude.
        if (p.lng == a.lng || p.lng == b.lng) {
            return _intersects(a, b, Point({p.lat, p.lng + kEpsilon}));
        }
        // Simple cases were intersection not possible 
        if (p.lng > b.lng || p.lng < a.lng || p.lat > std::max(a.lat, b.lat)) {
            return false;
        }
        // Simple case where intersection will always occur
        if (p.lat < std::min(a.lat, b.lat)) {
            return true;
        }
        // If A is at same latitude as P, then intersect
        // If A is as same latitude as B, then only intersect P is also as same latitude
        // Otherwise, if angle between A+P is greater than between A+B then an intersection will occur
        double angle_ap = std::abs(a.lat - p.lat) > kMin ? (p.lng - a.lng) / (p.lat - a.lat) : kMax;
        double angle_ab = std::abs(a.lat - b.lat) > kMin ? (b.lng - a.lng) / (b.lat - a.lat) : kMax;
        return angle_ap >= angle_ab;
    }
};