#include "TimelineSvgWriter.h"

#include "MathUtils.h"
#include "ShotAnalyzer.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <stdexcept>

namespace {

std::string phase_color(const std::string& phase) {
    if (phase == "setup") {
        return "#E8EEF9";
    }
    if (phase == "loading") {
        return "#FFF2CC";
    }
    if (phase == "release") {
        return "#F8D7DA";
    }
    return "#E2F4EA";
}

double chart_x(std::size_t index, std::size_t count, double left, double width) {
    if (count <= 1) {
        return left;
    }
    return left + width * static_cast<double>(index) / static_cast<double>(count - 1);
}

double chart_y(double value, double min_value, double max_value, double top, double height) {
    if (max_value <= min_value) {
        return top + height;
    }
    const double t = clamp((value - min_value) / (max_value - min_value), 0.0, 1.0);
    return top + height * (1.0 - t);
}

}  // namespace

void write_timeline_svg(const std::string& path, const std::vector<FrameMetrics>& metrics, const AnalysisResult& result) {
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("Cannot write timeline SVG: " + path);
    }

    const double svg_w = 960.0;
    const double svg_h = 560.0;
    const double left = 80.0;
    const double top = 110.0;
    const double plot_w = 820.0;
    const double plot_h = 330.0;
    const double bottom = top + plot_h;

    double max_wrist_speed = 0.0;
    for (const auto& m : metrics) {
        max_wrist_speed = std::max(max_wrist_speed, m.wrist_speed);
    }
    max_wrist_speed = std::max(max_wrist_speed, 1.0);

    out << std::fixed << std::setprecision(2);
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << svg_w << "\" height=\"" << svg_h << "\" viewBox=\"0 0 " << svg_w << " " << svg_h << "\">\n";
    out << "<rect width=\"100%\" height=\"100%\" fill=\"#FAFAFA\"/>\n";
    out << "<text x=\"40\" y=\"45\" font-family=\"Arial\" font-size=\"24\" font-weight=\"700\" fill=\"#1F2937\">Hockey Shot Timeline</text>\n";
    out << "<text x=\"40\" y=\"75\" font-family=\"Arial\" font-size=\"16\" fill=\"#4B5563\">Total score: " << result.scores.total
        << " / Goal: " << result.goal_direction.name
        << " / Release frame: " << result.values.release_frame << "</text>\n";

    for (std::size_t i = 0; i < metrics.size(); ++i) {
        const double x0 = chart_x(i, metrics.size(), left, plot_w);
        const double x1 = i + 1 < metrics.size() ? chart_x(i + 1, metrics.size(), left, plot_w) : left + plot_w;
        out << "<rect x=\"" << x0 << "\" y=\"" << top << "\" width=\"" << std::max(1.0, x1 - x0)
            << "\" height=\"" << plot_h << "\" fill=\"" << phase_color(phase_name(i, result.release_index))
            << "\" opacity=\"0.55\"/>\n";
    }

    out << "<line x1=\"" << left << "\" y1=\"" << top << "\" x2=\"" << left << "\" y2=\"" << bottom << "\" stroke=\"#6B7280\"/>\n";
    out << "<line x1=\"" << left << "\" y1=\"" << bottom << "\" x2=\"" << left + plot_w << "\" y2=\"" << bottom << "\" stroke=\"#6B7280\"/>\n";

    const double release_x = chart_x(result.release_index, metrics.size(), left, plot_w);
    out << "<line x1=\"" << release_x << "\" y1=\"" << top - 10 << "\" x2=\"" << release_x << "\" y2=\"" << bottom
        << "\" stroke=\"#DC2626\" stroke-width=\"2\" stroke-dasharray=\"6 4\"/>\n";
    out << "<text x=\"" << release_x + 8 << "\" y=\"" << top - 16 << "\" font-family=\"Arial\" font-size=\"13\" fill=\"#DC2626\">release</text>\n";

    auto write_polyline = [&](const std::string& color, const std::string& label, auto value_fn, double min_value, double max_value, double label_y) {
        out << "<polyline fill=\"none\" stroke=\"" << color << "\" stroke-width=\"3\" points=\"";
        for (std::size_t i = 0; i < metrics.size(); ++i) {
            const double x = chart_x(i, metrics.size(), left, plot_w);
            const double y = chart_y(value_fn(metrics[i]), min_value, max_value, top, plot_h);
            out << x << "," << y << " ";
        }
        out << "\"/>\n";
        out << "<rect x=\"720\" y=\"" << label_y - 13 << "\" width=\"16\" height=\"4\" fill=\"" << color << "\"/>\n";
        out << "<text x=\"742\" y=\"" << label_y << "\" font-family=\"Arial\" font-size=\"14\" fill=\"#374151\">" << label << "</text>\n";
    };

    write_polyline("#2563EB", "wrist speed", [](const FrameMetrics& m) { return m.wrist_speed; }, 0.0, max_wrist_speed, 470.0);
    write_polyline("#DC2626", "knee angle", [](const FrameMetrics& m) { return m.knee_angle_deg; }, 90.0, 180.0, 492.0);
    write_polyline("#059669", "torso lean", [](const FrameMetrics& m) { return m.torso_lean_deg; }, 0.0, 45.0, 514.0);

    out << "<text x=\"40\" y=\"465\" font-family=\"Arial\" font-size=\"13\" fill=\"#4B5563\">Phases: setup / loading / release / follow-through</text>\n";
    out << "<text x=\"40\" y=\"492\" font-family=\"Arial\" font-size=\"13\" fill=\"#4B5563\">Blue: wrist speed, Red: knee angle, Green: torso lean</text>\n";
    out << "</svg>\n";
}
