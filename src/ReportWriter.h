#pragma once

#include "Types.h"

#include <string>
#include <vector>

void write_metrics_csv(const std::string& path, const std::vector<FrameMetrics>& metrics, std::size_t release_index);
void write_report_json(const std::string& path, const AnalysisResult& result);
