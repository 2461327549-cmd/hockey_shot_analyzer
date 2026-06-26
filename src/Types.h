#pragma once

#include <cstddef>
#include <string>
#include <vector>

// 2D座標です。入力CSVでは0.0〜1.0の正規化座標を想定しています。
struct Point {
    double x = 0.0;
    double y = 0.0;
};

// 1フレーム分の骨格ランドマークです。
// 真横カメラなので、肩・腰・膝・足首・手首はカメラ側に見えている片側の点を使います。
struct LandmarkFrame {
    int frame = 0;
    double time = 0.0;
    Point shoulder;
    Point hip;
    Point knee;
    Point ankle;
    Point front_ankle;
    Point back_ankle;
    Point wrist;
};

// 1フレームごとに計算したフォーム指標です。
// 最終スコアだけでなく、あとでグラフ化や動画オーバーレイにも使えます。
struct FrameMetrics {
    int frame = 0;
    double time = 0.0;
    double knee_angle_deg = 0.0;  // 腰・膝・足首から作る膝角度
    double torso_lean_deg = 0.0;  // 肩・腰ラインの垂直方向からの傾き
    double com_x = 0.0;           // 肩と腰の中点を使った簡易重心のx座標
    double body_scale = 1.0;      // 肩から足首までの高さ。距離系指標の正規化に使います。
    double stance_width = 0.0;    // 身体サイズで正規化した前後スタンス幅
    double wrist_speed = 0.0;     // 身体サイズで正規化した手首のフレーム間速度
};

// 6つの評価項目と総合スコアです。
struct Scores {
    double knee = 0.0;
    double weight_transfer = 0.0;
    double stance = 0.0;
    double torso = 0.0;
    double wrist = 0.0;
    double follow_through = 0.0;
    double total = 0.0;
};

// スコアの根拠になる実測値です。
// report.jsonに出すことで「なぜこの点数になったか」を確認できます。
struct MeasuredValues {
    int release_frame = 0;
    double release_time = 0.0;
    double knee_angle_deg = 0.0;
    double body_scale = 1.0;
    double weight_transfer = 0.0;
    double stance_width = 0.0;
    double torso_lean_deg = 0.0;
    double max_wrist_speed = 0.0;
    double follow_through_x = 0.0;
    double follow_through_y_drop = 0.0;
};

// 画面上でゴールが右にあるか左にあるかを表します。
struct GoalDirection {
    std::string name = "right";
    double x_sign = 1.0;
};

struct ScoreBand {
    double ideal_low = 0.0;
    double ideal_high = 0.0;
    double tolerance = 1.0;
};

// 採点基準です。configファイルで上書きできます。
struct ScoringConfig {
    ScoreBand knee{120.0, 145.0, 35.0};
    ScoreBand weight_transfer{0.05, 0.16, 0.08};
    ScoreBand stance{0.22, 0.42, 0.18};
    ScoreBand torso{10.0, 30.0, 20.0};
    ScoreBand wrist{2.5, 8.0, 2.0};
    ScoreBand follow_direction{0.07, 0.35, 0.15};
    double follow_y_drop_limit = 0.14;
    double follow_y_drop_penalty = 250.0;

    double weight_knee = 0.20;
    double weight_transfer_score = 0.25;
    double weight_stance = 0.15;
    double weight_torso = 0.15;
    double weight_wrist = 0.15;
    double weight_follow = 0.10;
};

// 採点結果、実測値、改善コメントをまとめた最終解析結果です。
struct AnalysisResult {
    GoalDirection goal_direction;
    std::size_t release_index = 0;
    Scores scores;
    MeasuredValues values;
    std::vector<std::string> advice;
};
