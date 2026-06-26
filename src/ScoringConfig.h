#pragma once

#include "Types.h"

#include <string>

ScoringConfig default_scoring_config();
ScoringConfig load_scoring_config(const std::string& path);
