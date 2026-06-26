#include "MetricsCalculator.h"

#include "MathUtils.h"

#include <cmath>

namespace {

double body_scale_from_landmarks(const LandmarkFrame& row) {
    const double scale = std::abs(row.ankle.y - row.shoulder.y);
    return scale > 0.001 ? scale : 1.0;
}

double angle_deg(Point a, Point b, Point c) {
    const double bax = a.x - b.x;
    const double bay = a.y - b.y;
    const double bcx = c.x - b.x;
    const double bcy = c.y - b.y;
    const double dot = bax * bcx + bay * bcy;
    const double len_a = std::sqrt(bax * bax + bay * bay);
    const double len_c = std::sqrt(bcx * bcx + bcy * bcy);
    if (len_a == 0.0 || len_c == 0.0) {
        return 0.0;
    }
    const double cos_theta = clamp(dot / (len_a * len_c), -1.0, 1.0);
    return std::acos(cos_theta) * 180.0 / M_PI;
}

double torso_lean_from_vertical_deg(Point shoulder, Point hip) {
    const double dx = shoulder.x - hip.x;
    const double dy = shoulder.y - hip.y;
    if (dx == 0.0 && dy == 0.0) {
        return 0.0;
    }
    return std::atan2(std::abs(dx), std::abs(dy)) * 180.0 / M_PI;
}

}  // namespace

std::vector<FrameMetrics> calculate_metrics(const std::vector<LandmarkFrame>& frames) {
    std::vector<FrameMetrics> metrics;
    metrics.reserve(frames.size());

    for (std::size_t i = 0; i < frames.size(); ++i) {
        const auto& row = frames[i];
        FrameMetrics m;
        m.frame = row.frame;
        m.time = row.time;
        m.knee_angle_deg = angle_deg(row.hip, row.knee, row.ankle);
        m.torso_lean_deg = torso_lean_from_vertical_deg(row.shoulder, row.hip);
        m.com_x = (row.shoulder.x + row.hip.x) / 2.0;
        m.body_scale = body_scale_from_landmarks(row);
        m.stance_width = std::abs(row.front_ankle.x - row.back_ankle.x) / m.body_scale;

        if (i > 0) {
            const double dt = row.time - frames[i - 1].time;
            m.wrist_speed = dt > 0.0 ? distance(row.wrist, frames[i - 1].wrist) / dt / m.body_scale : 0.0;
        }
        metrics.push_back(m);
    }

    return metrics;
}
