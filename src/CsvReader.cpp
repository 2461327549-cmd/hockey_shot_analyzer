#include "CsvReader.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

const std::vector<std::string> kExpectedHeader = {
    "frame",
    "time",
    "shoulder_x",
    "shoulder_y",
    "hip_x",
    "hip_y",
    "knee_x",
    "knee_y",
    "ankle_x",
    "ankle_y",
    "front_ankle_x",
    "front_ankle_y",
    "back_ankle_x",
    "back_ankle_y",
    "wrist_x",
    "wrist_y",
};

std::string trim(const std::string& value) {
    const std::size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    const std::size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::vector<std::string> split_csv_line(const std::string& line) {
    std::vector<std::string> fields;
    std::stringstream ss(line);
    std::string field;
    while (std::getline(ss, field, ',')) {
        fields.push_back(field);
    }
    return fields;
}

void validate_header(const std::string& header_line) {
    auto fields = split_csv_line(header_line);
    if (fields.size() != kExpectedHeader.size()) {
        throw std::runtime_error(
            "CSV header has " + std::to_string(fields.size()) + " columns; expected " + std::to_string(kExpectedHeader.size())
        );
    }

    for (std::size_t i = 0; i < kExpectedHeader.size(); ++i) {
        fields[i] = trim(fields[i]);
        if (fields[i] != kExpectedHeader[i]) {
            throw std::runtime_error(
                "CSV header column " + std::to_string(i + 1) + " is '" + fields[i] + "', expected '" + kExpectedHeader[i] + "'"
            );
        }
    }
}

double to_double(const std::vector<std::string>& fields, std::size_t index, int line_number, const std::string& column_name) {
    if (index >= fields.size()) {
        throw std::runtime_error("CSV line " + std::to_string(line_number) + " is missing column '" + column_name + "'");
    }
    try {
        return std::stod(trim(fields[index]));
    } catch (const std::exception&) {
        throw std::runtime_error(
            "CSV line " + std::to_string(line_number) + " column '" + column_name + "' is not a valid number: '" + fields[index] + "'"
        );
    }
}

double value_at(const std::vector<std::string>& fields, std::size_t index, int line_number) {
    return to_double(fields, index, line_number, kExpectedHeader[index]);
}

}  // namespace

std::vector<LandmarkFrame> read_landmarks(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Cannot open input CSV: " + path);
    }

    std::vector<LandmarkFrame> frames;
    std::string line;
    if (!std::getline(in, line)) {
        throw std::runtime_error("CSV is empty: " + path);
    }
    validate_header(line);

    int line_number = 1;
    while (std::getline(in, line)) {
        ++line_number;
        if (line.empty()) {
            continue;
        }
        const auto f = split_csv_line(line);
        if (f.size() != kExpectedHeader.size()) {
            throw std::runtime_error(
                "CSV line " + std::to_string(line_number) + " has " + std::to_string(f.size())
                + " columns; expected " + std::to_string(kExpectedHeader.size())
            );
        }

        LandmarkFrame row;
        row.frame = static_cast<int>(value_at(f, 0, line_number));
        row.time = value_at(f, 1, line_number);
        row.shoulder = {value_at(f, 2, line_number), value_at(f, 3, line_number)};
        row.hip = {value_at(f, 4, line_number), value_at(f, 5, line_number)};
        row.knee = {value_at(f, 6, line_number), value_at(f, 7, line_number)};
        row.ankle = {value_at(f, 8, line_number), value_at(f, 9, line_number)};
        row.front_ankle = {value_at(f, 10, line_number), value_at(f, 11, line_number)};
        row.back_ankle = {value_at(f, 12, line_number), value_at(f, 13, line_number)};
        row.wrist = {value_at(f, 14, line_number), value_at(f, 15, line_number)};
        frames.push_back(row);
    }

    if (frames.size() < 2) {
        throw std::runtime_error("At least two landmark frames are required");
    }
    return frames;
}
