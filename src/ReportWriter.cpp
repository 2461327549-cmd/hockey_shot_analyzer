#include "ReportWriter.h"

#include "ShotAnalyzer.h"

#include <fstream>
#include <iomanip>
#include <stdexcept>

void write_metrics_csv(const std::string& path, const std::vector<FrameMetrics>& metrics, std::size_t release_index) {
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("Cannot write metrics CSV: " + path);
    }
    out << "frame,time,phase,knee_angle_deg,torso_lean_deg,com_x,body_scale,stance_width,wrist_speed\n";
    out << std::fixed << std::setprecision(6);
    for (std::size_t i = 0; i < metrics.size(); ++i) {
        const auto& m = metrics[i];
        out << m.frame << ','
            << m.time << ','
            << phase_name(i, release_index) << ','
            << m.knee_angle_deg << ','
            << m.torso_lean_deg << ','
            << m.com_x << ','
            << m.body_scale << ','
            << m.stance_width << ','
            << m.wrist_speed << '\n';
    }
}

void write_report_json(const std::string& path, const AnalysisResult& result) {
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("Cannot write report JSON: " + path);
    }
    const Scores& s = result.scores;
    const MeasuredValues& v = result.values;

    out << std::fixed << std::setprecision(2);
    out << "{\n";
    out << "  \"total_score\": " << s.total << ",\n";
    out << "  \"goal_direction\": \"" << result.goal_direction.name << "\",\n";
    out << "  \"release\": {\n";
    out << "    \"frame\": " << v.release_frame << ",\n";
    out << "    \"time\": " << v.release_time << "\n";
    out << "  },\n";
    out << "  \"scores\": {\n";
    out << "    \"knee_flexion\": " << s.knee << ",\n";
    out << "    \"weight_transfer\": " << s.weight_transfer << ",\n";
    out << "    \"stance_width\": " << s.stance << ",\n";
    out << "    \"torso_lean\": " << s.torso << ",\n";
    out << "    \"wrist_snap_timing\": " << s.wrist << ",\n";
    out << "    \"follow_through\": " << s.follow_through << "\n";
    out << "  },\n";
    out << "  \"measured_values\": {\n";
    out << "    \"knee_angle_deg\": " << v.knee_angle_deg << ",\n";
    out << "    \"body_scale\": " << v.body_scale << ",\n";
    out << "    \"weight_transfer\": " << v.weight_transfer << ",\n";
    out << "    \"stance_width\": " << v.stance_width << ",\n";
    out << "    \"torso_lean_deg\": " << v.torso_lean_deg << ",\n";
    out << "    \"max_wrist_speed\": " << v.max_wrist_speed << ",\n";
    out << "    \"follow_through_x\": " << v.follow_through_x << ",\n";
    out << "    \"follow_through_y_drop\": " << v.follow_through_y_drop << "\n";
    out << "  },\n";
    out << "  \"advice\": [\n";
    for (std::size_t i = 0; i < result.advice.size(); ++i) {
        out << "    \"" << result.advice[i] << "\"";
        if (i + 1 < result.advice.size()) {
            out << ",";
        }
        out << "\n";
    }
    out << "  ]\n";
    out << "}\n";
}
