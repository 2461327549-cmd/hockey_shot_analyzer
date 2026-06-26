#include "HtmlReportWriter.h"

#include "MathUtils.h"

#include <fstream>
#include <iomanip>
#include <stdexcept>

namespace {

void write_score_row(std::ofstream& out, const std::string& label, double score) {
    out << "<tr><td>" << label << "</td><td>" << score << "</td><td><div class=\"bar\"><span style=\"width:"
        << clamp(score, 0.0, 100.0) << "%\"></span></div></td></tr>\n";
}

std::string file_name_from_path(const std::string& path) {
    const std::size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) {
        return path;
    }
    return path.substr(pos + 1);
}

}  // namespace

void write_html_report(const std::string& path, const std::string& timeline_svg_path, const AnalysisResult& result) {
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("Cannot write HTML report: " + path);
    }

    const Scores& s = result.scores;
    const MeasuredValues& v = result.values;

    out << std::fixed << std::setprecision(2);
    out << "<!doctype html>\n<html lang=\"ja\">\n<head>\n<meta charset=\"utf-8\">\n";
    out << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n";
    out << "<title>Hockey Shot Report</title>\n";
    out << "<style>\n";
    out << "body{font-family:Arial,'Hiragino Sans','Yu Gothic',sans-serif;margin:0;background:#f6f7f9;color:#1f2937;}\n";
    out << "main{max-width:1040px;margin:0 auto;padding:32px 20px;}\n";
    out << "header{display:flex;justify-content:space-between;gap:24px;align-items:flex-end;margin-bottom:24px;}\n";
    out << "h1{font-size:28px;margin:0 0 8px;} h2{font-size:18px;margin:0 0 14px;} p{margin:0;color:#4b5563;}\n";
    out << ".score{font-size:48px;font-weight:800;color:#0f766e;line-height:1;}\n";
    out << ".grid{display:grid;grid-template-columns:1.1fr .9fr;gap:20px;} section{background:#fff;border:1px solid #e5e7eb;border-radius:8px;padding:20px;}\n";
    out << "table{width:100%;border-collapse:collapse;font-size:14px;} td{padding:9px 0;border-bottom:1px solid #eef0f3;vertical-align:middle;} td:nth-child(2){width:64px;text-align:right;font-weight:700;}\n";
    out << ".bar{height:8px;background:#e5e7eb;border-radius:999px;overflow:hidden;margin-left:14px;} .bar span{display:block;height:100%;background:#2563eb;}\n";
    out << ".facts{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px;} .fact{background:#f9fafb;border:1px solid #edf0f2;border-radius:6px;padding:10px;}\n";
    out << ".fact b{display:block;font-size:12px;color:#6b7280;margin-bottom:4px;} .fact span{font-size:17px;font-weight:700;}\n";
    out << "ul{margin:0;padding-left:20px;} li{margin:8px 0;line-height:1.5;} img{width:100%;height:auto;border:1px solid #e5e7eb;border-radius:8px;background:#fff;}\n";
    out << ".timeline{grid-column:1/-1;} @media(max-width:800px){header{display:block}.grid{grid-template-columns:1fr}.score{margin-top:16px}}\n";
    out << "</style>\n</head>\n<body>\n<main>\n";
    out << "<header><div><h1>Hockey Shot Report</h1><p>Side-view wrist shot analysis / Goal direction: "
        << result.goal_direction.name << " / Release frame: " << v.release_frame << "</p></div>";
    out << "<div><div class=\"score\">" << s.total << "</div><p>Total score</p></div></header>\n";

    out << "<div class=\"grid\">\n<section><h2>Scores</h2><table>\n";
    write_score_row(out, "Knee flexion", s.knee);
    write_score_row(out, "Weight transfer", s.weight_transfer);
    write_score_row(out, "Stance width", s.stance);
    write_score_row(out, "Torso lean", s.torso);
    write_score_row(out, "Wrist snap timing", s.wrist);
    write_score_row(out, "Follow-through", s.follow_through);
    out << "</table></section>\n";

    out << "<section><h2>Measured Values</h2><div class=\"facts\">";
    out << "<div class=\"fact\"><b>Release time</b><span>" << v.release_time << " s</span></div>";
    out << "<div class=\"fact\"><b>Knee angle</b><span>" << v.knee_angle_deg << " deg</span></div>";
    out << "<div class=\"fact\"><b>Body scale</b><span>" << v.body_scale << "</span></div>";
    out << "<div class=\"fact\"><b>Weight transfer</b><span>" << v.weight_transfer << "</span></div>";
    out << "<div class=\"fact\"><b>Stance width</b><span>" << v.stance_width << "</span></div>";
    out << "<div class=\"fact\"><b>Torso lean</b><span>" << v.torso_lean_deg << " deg</span></div>";
    out << "<div class=\"fact\"><b>Max wrist speed</b><span>" << v.max_wrist_speed << "</span></div>";
    out << "</div></section>\n";

    out << "<section><h2>Advice</h2><ul>";
    for (const auto& item : result.advice) {
        out << "<li>" << item << "</li>";
    }
    out << "</ul></section>\n";

    out << "<section><h2>Release And Follow-through</h2><div class=\"facts\">";
    out << "<div class=\"fact\"><b>Follow-through x</b><span>" << v.follow_through_x << "</span></div>";
    out << "<div class=\"fact\"><b>Follow-through y drop</b><span>" << v.follow_through_y_drop << "</span></div>";
    out << "</div></section>\n";

    if (!timeline_svg_path.empty()) {
        out << "<section class=\"timeline\"><h2>Timeline</h2><img src=\"" << file_name_from_path(timeline_svg_path) << "\" alt=\"Hockey shot timeline\"></section>\n";
    }

    out << "</div>\n</main>\n</body>\n</html>\n";
}
