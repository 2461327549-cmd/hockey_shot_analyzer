#pragma once

#include "Types.h"

#include <string>

void write_html_report(const std::string& path, const std::string& timeline_svg_path, const AnalysisResult& result);
