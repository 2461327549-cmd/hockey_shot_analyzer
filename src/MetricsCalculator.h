#pragma once

#include "Types.h"

#include <vector>

std::vector<FrameMetrics> calculate_metrics(const std::vector<LandmarkFrame>& frames);
