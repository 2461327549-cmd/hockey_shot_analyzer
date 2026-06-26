#include "ScoringConfig.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

std::string trim(const std::string& value) {
    const std::size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    const std::size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

bool set_band_value(ScoreBand& band, const std::string& field, double value) {
    if (field == "low") {
        band.ideal_low = value;
        return true;
    }
    if (field == "high") {
        band.ideal_high = value;
        return true;
    }
    if (field == "tolerance") {
        band.tolerance = value;
        return true;
    }
    return false;
}

void set_config_value(ScoringConfig& config, const std::string& key, double value) {
    const std::size_t dot = key.find('.');
    if (dot == std::string::npos) {
        if (key == "follow_y_drop_limit") {
            config.follow_y_drop_limit = value;
            return;
        }
        if (key == "follow_y_drop_penalty") {
            config.follow_y_drop_penalty = value;
            return;
        }
        throw std::runtime_error("Unknown config key: " + key);
    }

    const std::string group = key.substr(0, dot);
    const std::string field = key.substr(dot + 1);

    if (group == "knee" && set_band_value(config.knee, field, value)) return;
    if (group == "weight_transfer" && set_band_value(config.weight_transfer, field, value)) return;
    if (group == "stance" && set_band_value(config.stance, field, value)) return;
    if (group == "torso" && set_band_value(config.torso, field, value)) return;
    if (group == "wrist" && set_band_value(config.wrist, field, value)) return;
    if (group == "follow_direction" && set_band_value(config.follow_direction, field, value)) return;

    if (group == "weights") {
        if (field == "knee") {
            config.weight_knee = value;
            return;
        }
        if (field == "weight_transfer") {
            config.weight_transfer_score = value;
            return;
        }
        if (field == "stance") {
            config.weight_stance = value;
            return;
        }
        if (field == "torso") {
            config.weight_torso = value;
            return;
        }
        if (field == "wrist") {
            config.weight_wrist = value;
            return;
        }
        if (field == "follow") {
            config.weight_follow = value;
            return;
        }
    }

    throw std::runtime_error("Unknown config key: " + key);
}

}  // namespace

ScoringConfig default_scoring_config() {
    return ScoringConfig{};
}

ScoringConfig load_scoring_config(const std::string& path) {
    ScoringConfig config = default_scoring_config();
    if (path.empty()) {
        return config;
    }

    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Cannot open scoring config: " + path);
    }

    std::string line;
    int line_number = 0;
    while (std::getline(in, line)) {
        ++line_number;
        const std::size_t comment = line.find('#');
        if (comment != std::string::npos) {
            line = line.substr(0, comment);
        }
        line = trim(line);
        if (line.empty()) {
            continue;
        }

        const std::size_t equals = line.find('=');
        if (equals == std::string::npos) {
            throw std::runtime_error("Invalid config line " + std::to_string(line_number) + ": " + line);
        }

        const std::string key = trim(line.substr(0, equals));
        const std::string value_text = trim(line.substr(equals + 1));
        set_config_value(config, key, std::stod(value_text));
    }

    return config;
}
