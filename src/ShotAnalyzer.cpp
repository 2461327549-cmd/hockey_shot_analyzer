#include "ShotAnalyzer.h"

#include "MathUtils.h"

#include <algorithm>
#include <stdexcept>

namespace {

double band_score(double value, const ScoreBand& band) {
    if (value >= band.ideal_low && value <= band.ideal_high) {
        return 100.0;
    }
    const double diff = value < band.ideal_low ? band.ideal_low - value : value - band.ideal_high;
    return clamp(100.0 * (1.0 - diff / band.tolerance), 0.0, 100.0);
}

void add_advice_if_low(std::vector<std::string>& advice, double score, const std::string& message) {
    if (score < 70.0) {
        advice.push_back(message);
    }
}

}  // namespace

bool is_goal_direction(const std::string& value) {
    return value == "right" || value == "left";
}

GoalDirection parse_goal_direction(const std::string& value) {
    if (value == "right") {
        return {"right", 1.0};
    }
    if (value == "left") {
        return {"left", -1.0};
    }
    throw std::runtime_error("Goal direction must be 'right' or 'left'");
}

std::size_t find_release_index(const std::vector<FrameMetrics>& metrics) {
    const auto release_it = std::max_element(metrics.begin(), metrics.end(), [](const auto& a, const auto& b) {
        return a.wrist_speed < b.wrist_speed;
    });
    return static_cast<std::size_t>(std::distance(metrics.begin(), release_it));
}

std::string phase_name(std::size_t frame_index, std::size_t release_index) {
    if (frame_index == release_index) {
        return "release";
    }
    if (frame_index > release_index) {
        return "follow_through";
    }

    const std::size_t setup_end = release_index / 3;
    if (frame_index <= setup_end) {
        return "setup";
    }
    return "loading";
}

AnalysisResult analyze_shot(
    const std::vector<LandmarkFrame>& frames,
    const std::vector<FrameMetrics>& metrics,
    GoalDirection goal_direction,
    const ScoringConfig& config
) {
    const std::size_t release_index = find_release_index(metrics);

    const FrameMetrics& setup = metrics.front();
    const FrameMetrics& release = metrics[release_index];
    AnalysisResult result;
    result.goal_direction = goal_direction;
    result.release_index = release_index;
    Scores& s = result.scores;
    MeasuredValues& values = result.values;

    values.release_frame = release.frame;
    values.release_time = release.time;
    values.knee_angle_deg = release.knee_angle_deg;
    values.body_scale = release.body_scale;

    s.knee = band_score(release.knee_angle_deg, config.knee);

    const double weight_transfer = (release.com_x - setup.com_x) * goal_direction.x_sign / release.body_scale;
    values.weight_transfer = weight_transfer;
    s.weight_transfer = band_score(weight_transfer, config.weight_transfer);

    values.stance_width = release.stance_width;
    s.stance = band_score(release.stance_width, config.stance);

    values.torso_lean_deg = release.torso_lean_deg;
    s.torso = band_score(release.torso_lean_deg, config.torso);

    const double max_wrist_speed = release.wrist_speed;
    values.max_wrist_speed = max_wrist_speed;
    s.wrist = band_score(max_wrist_speed, config.wrist);

    const double follow_x = (frames.back().wrist.x - frames[release_index].wrist.x) * goal_direction.x_sign / release.body_scale;
    const double follow_y_drop = (frames.back().wrist.y - frames[release_index].wrist.y) / release.body_scale;
    values.follow_through_x = follow_x;
    values.follow_through_y_drop = follow_y_drop;
    const double follow_direction = band_score(follow_x, config.follow_direction);
    const double follow_height = follow_y_drop <= config.follow_y_drop_limit
        ? 100.0
        : clamp(100.0 - (follow_y_drop - config.follow_y_drop_limit) * config.follow_y_drop_penalty, 0.0, 100.0);
    s.follow_through = (follow_direction + follow_height) / 2.0;

    s.total = s.knee * config.weight_knee
        + s.weight_transfer * config.weight_transfer_score
        + s.stance * config.weight_stance
        + s.torso * config.weight_torso
        + s.wrist * config.weight_wrist
        + s.follow_through * config.weight_follow;

    add_advice_if_low(result.advice, s.knee, "膝の曲がりが浅い可能性があります。セットアップからリリース前にもう少し沈み込む意識を持つとよいです。");
    add_advice_if_low(result.advice, s.weight_transfer, "後ろ足から前足への体重移動が小さい可能性があります。リリースに向けて腰をゴール方向へ運ぶ動きを確認してください。");
    add_advice_if_low(result.advice, s.stance, "スタンス幅が理想範囲から外れています。前後の足幅が狭すぎる、または広すぎる可能性があります。");
    add_advice_if_low(result.advice, s.torso, "上半身の前傾が少ない、または倒れ込みすぎている可能性があります。肩と腰のラインを安定させてください。");
    add_advice_if_low(result.advice, s.wrist, "手首の加速が弱い可能性があります。体重移動のあとに手首を素早く走らせる順序を意識してください。");
    add_advice_if_low(result.advice, s.follow_through, "フォロースルーが短い、または手首が下に落ちている可能性があります。リリース後もゴール方向へ押し出してください。");

    if (result.advice.empty()) {
        result.advice.push_back("全体的に安定しています。次は実動画で複数本を比較し、再現性を確認するとよいです。");
    }

    return result;
}
