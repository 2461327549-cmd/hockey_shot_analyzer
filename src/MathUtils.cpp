#include "MathUtils.h"

#include <algorithm>
#include <cmath>

double clamp(double value, double low, double high) {
    return std::max(low, std::min(value, high));
}

double distance(Point a, Point b) {
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}
