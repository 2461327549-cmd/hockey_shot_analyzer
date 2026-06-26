#pragma once

#include "Types.h"

#include <string>
#include <vector>

void write_timeline_svg(const std::string& path, const std::vector<FrameMetrics>& metrics, const AnalysisResult& result);
