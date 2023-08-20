#pragma once
#include <vector>
#include <algorithm>    // min_element
#include <limits>       // numeric_limits
#include "point.hpp"

struct Polygon {
    std::vector<Point> outer;
    std::vector<std::vector<Point>> inner;
    std::pair<Point, Point> bounding_box;

    inline double max_x() const {return bounding_box.second.x;}
    inline double max_y() const {return bounding_box.second.y;}
    inline double min_x() const {return bounding_box.first.x;}
    inline double min_y() const {return bounding_box.first.y;}

    void shift(double x_shift, double y_shift) {
        bounding_box.first.shift(x_shift,y_shift);
        bounding_box.second.shift(x_shift,y_shift);
        for (auto& point : outer) {
            point.shift(x_shift,y_shift);
        }
        for (auto& inner_poly : inner) {
            for (auto& point : inner_poly) {
                point.shift(x_shift,y_shift);
            }
        }
    }

    void scale(double x_scale, double y_scale) {
        bounding_box.first.scale(x_scale,y_scale);
        bounding_box.second.scale(x_scale,y_scale);
        for (auto& point : outer) {
            point.scale(x_scale,y_scale);
        }
        for (auto& inner_poly : inner) {
            for (auto& point : inner_poly) {
                point.scale(x_scale,y_scale);
            }
        }
    }

    bool contains(const Point& p) const {
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

        // Find the point with lowest y (if y is the same, find with lowest x)
        auto b = std::min_element(start,stop);
        // Get point before and after the lowest point
        // Need to handle wrapping at edges of vector
        // First and last points are always equal so wrap to second to last point
        auto a = (b == start) ? (stop - 2) : b - 1;
        // First and last points are always equal so wrap to second from first point
        auto c = (b == (stop - 1)) ? (start + 1) : b + 1;

        double det = ((b->x - a->x) * (c->y - a->y)) - ((c->x - a->x) * (b->y - a->y));

        return det < 0;
    }

    static std::pair<Point, Point> get_bounding_box(std::vector<Point>::const_iterator start,
                                                  std::vector<Point>::const_iterator stop) {

        // Make min the maximum possible value, and max the min possible value
        Point min({std::numeric_limits<double>().max(), std::numeric_limits<double>().max()});
        Point max({std::numeric_limits<double>().lowest(), std::numeric_limits<double>().lowest()});

        auto it = start;
        while (it != stop) {
            if (it->y < min.y) min.y = it->y;
            if (it->x < min.x) min.x = it->x;
            if (it->y > max.y) max.y = it->y;
            if (it->x > max.x) max.x = it->x;
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

        // Algorithm works when a x is less than or equal to b x
        // So flip vertices around
        if (a.x > b.x) {
            return _intersects(b, a, p);
        }
        // Algorithm only works when p x is not the same as a or b.
        // So add a very small value to p x.
        if (p.x == a.x || p.x == b.x) {
            return _intersects(a, b, Point({p.y, p.x + kEpsilon}));
        }
        // Simple cases were intersection not possible 
        if (p.x > b.x || p.x < a.x || p.y > std::max(a.y, b.y)) {
            return false;
        }
        // Simple case where intersection will always occur
        if (p.y < std::min(a.y, b.y)) {
            return true;
        }
        // If A is at same y as P, then intersect
        // If A is as same y as B, then only intersect P is also as same y
        // Otherwise, if angle between A+P is greater than between A+B then an intersection will occur
        double angle_ap = std::abs(a.y - p.y) > kMin ? (p.x - a.x) / (p.y - a.y) : kMax;
        double angle_ab = std::abs(a.y - b.y) > kMin ? (b.x - a.x) / (b.y - a.y) : kMax;
        return angle_ap >= angle_ab;
    }
};