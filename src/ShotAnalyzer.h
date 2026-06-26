#pragma once

#include "Types.h"

#include <string>
#include <vector>

bool is_goal_direction(const std::string& value);
GoalDirection parse_goal_direction(const std::string& value);
std::size_t find_release_index(const std::vector<FrameMetrics>& metrics);
std::string phase_name(std::size_t frame_index, std::size_t release_index);
AnalysisResult analyze_shot(
    const std::vector<LandmarkFrame>& frames,
    const std::vector<FrameMetrics>& metrics,
    GoalDirection goal_direction,
    const ScoringConfig& config
);
