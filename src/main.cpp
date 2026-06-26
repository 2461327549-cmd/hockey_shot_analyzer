#include "CsvReader.h"
#include "HtmlReportWriter.h"
#include "MetricsCalculator.h"
#include "ReportWriter.h"
#include "ScoringConfig.h"
#include "ShotAnalyzer.h"
#include "TimelineSvgWriter.h"

#include <exception>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

void print_usage(const char* program_name) {
    std::cout
        << "Usage: " << program_name << " <landmarks.csv> <metrics.csv> <report.json> [right|left] [timeline.svg] [report.html] [scoring.conf]\n"
        << "\n"
        << "Example:\n"
        << "  " << program_name << " samples/landmarks.csv outputs/metrics.csv outputs/report.json right outputs/timeline.svg outputs/report.html config/scoring.conf\n";
}

void ensure_parent_directory(const std::string& path) {
    if (path.empty()) {
        return;
    }
    const std::filesystem::path output_path(path);
    const auto parent = output_path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
}

}  // namespace

int main(int argc, char** argv) {
    // 使い方:
    // ./hockey_shot_analyzer <入力landmarks.csv> <出力metrics.csv> <出力report.json> [right|left] [timeline.svg] [report.html] [scoring.conf]
    if (argc == 2 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
        print_usage(argv[0]);
        return 0;
    }

    if (argc < 4 || argc > 8) {
        print_usage(argv[0]);
        return 1;
    }

    try {
        std::string goal_direction_text = "right";
        std::string timeline_svg_path;
        std::string html_report_path;
        std::string scoring_config_path;
        if (argc >= 5) {
            if (is_goal_direction(argv[4])) {
                goal_direction_text = argv[4];
                if (argc >= 6) {
                    timeline_svg_path = argv[5];
                }
                if (argc >= 7) {
                    html_report_path = argv[6];
                }
                if (argc == 8) {
                    scoring_config_path = argv[7];
                }
            } else {
                timeline_svg_path = argv[4];
                if (argc >= 6) {
                    html_report_path = argv[5];
                }
                if (argc >= 7) {
                    scoring_config_path = argv[6];
                }
            }
        }

        const GoalDirection goal_direction = parse_goal_direction(goal_direction_text);
        const ScoringConfig scoring_config = load_scoring_config(scoring_config_path);
        const auto landmarks = read_landmarks(argv[1]);
        const auto metrics = calculate_metrics(landmarks);
        const auto result = analyze_shot(landmarks, metrics, goal_direction, scoring_config);

        ensure_parent_directory(argv[2]);
        ensure_parent_directory(argv[3]);
        ensure_parent_directory(timeline_svg_path);
        ensure_parent_directory(html_report_path);

        write_metrics_csv(argv[2], metrics, result.release_index);
        write_report_json(argv[3], result);
        if (!timeline_svg_path.empty()) {
            write_timeline_svg(timeline_svg_path, metrics, result);
        }
        if (!html_report_path.empty()) {
            write_html_report(html_report_path, timeline_svg_path, result);
        }

        std::cout << "Analysis complete. Total score: " << std::fixed << std::setprecision(2)
                  << result.scores.total << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
