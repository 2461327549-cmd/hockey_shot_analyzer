#include <opencv2/dnn.hpp>
#include <opencv2/opencv.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr int kInputSize = 640;
constexpr int kKeypointCount = 17;
constexpr int kValuesPerKeypoint = 3;

enum CocoKeypoint {
    LeftShoulder = 5,
    RightShoulder = 6,
    LeftWrist = 9,
    RightWrist = 10,
    LeftHip = 11,
    RightHip = 12,
    LeftKnee = 13,
    RightKnee = 14,
    LeftAnkle = 15,
    RightAnkle = 16,
};

struct Keypoint {
    float x = 0.0f;
    float y = 0.0f;
    float confidence = 0.0f;
};

struct Detection {
    float score = 0.0f;
    std::array<Keypoint, kKeypointCount> keypoints;
};

struct LetterboxInfo {
    float scale = 1.0f;
    int pad_x = 0;
    int pad_y = 0;
};

struct ExportRow {
    int frame = 0;
    double time = 0.0;
    Keypoint shoulder;
    Keypoint hip;
    Keypoint knee;
    Keypoint ankle;
    Keypoint front_ankle;
    Keypoint back_ankle;
    Keypoint wrist;
};

void print_usage(const char* program_name) {
    std::cout
        << "Usage: " << program_name << " <video> <yolov8_pose.onnx> <landmarks.csv> [right|left]\n\n"
        << "Uses OpenCV DNN. Model expectation: YOLOv8-style pose ONNX output with 17 COCO keypoints.\n";
}

bool is_goal_direction(const std::string& value) {
    return value == "right" || value == "left";
}

void ensure_parent_directory(const std::string& path) {
    const std::filesystem::path output_path(path);
    const auto parent = output_path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
}

float sigmoid(float x) {
    return 1.0f / (1.0f + std::exp(-x));
}

cv::Mat letterbox(const cv::Mat& frame, LetterboxInfo& info) {
    const int width = frame.cols;
    const int height = frame.rows;
    info.scale = std::min(static_cast<float>(kInputSize) / static_cast<float>(width), static_cast<float>(kInputSize) / static_cast<float>(height));
    const int resized_w = static_cast<int>(std::round(width * info.scale));
    const int resized_h = static_cast<int>(std::round(height * info.scale));
    info.pad_x = (kInputSize - resized_w) / 2;
    info.pad_y = (kInputSize - resized_h) / 2;

    cv::Mat resized;
    cv::resize(frame, resized, cv::Size(resized_w, resized_h));

    cv::Mat canvas(kInputSize, kInputSize, CV_8UC3, cv::Scalar(114, 114, 114));
    resized.copyTo(canvas(cv::Rect(info.pad_x, info.pad_y, resized_w, resized_h)));
    return canvas;
}

std::vector<int> output_shape(const cv::Mat& output) {
    std::vector<int> shape;
    for (int i = 0; i < output.dims; ++i) {
        shape.push_back(output.size[i]);
    }
    return shape;
}

std::vector<Detection> decode_yolov8_pose(
    const cv::Mat& output,
    const LetterboxInfo& letterbox_info,
    int frame_width,
    int frame_height,
    float score_threshold
) {
    const std::vector<int> shape = output_shape(output);
    if (shape.size() != 2 && shape.size() != 3) {
        throw std::runtime_error("Expected YOLO pose output with 2 or 3 dimensions");
    }

    int channels = 0;
    int candidates = 0;
    bool channel_first = true;

    if (shape.size() == 3) {
        channels = shape[1];
        candidates = shape[2];
        channel_first = true;
        if (shape[2] == 56) {
            channels = shape[2];
            candidates = shape[1];
            channel_first = false;
        }
    } else {
        channels = shape[0];
        candidates = shape[1];
        channel_first = true;
        if (shape[1] == 56) {
            channels = shape[1];
            candidates = shape[0];
            channel_first = false;
        }
    }

    if (channels < 5 + kKeypointCount * kValuesPerKeypoint) {
        throw std::runtime_error("Unexpected pose output channel count");
    }

    const float* data = output.ptr<float>();
    auto at = [&](int candidate, int channel) -> float {
        if (channel_first) {
            return data[channel * candidates + candidate];
        }
        return data[candidate * channels + channel];
    };

    std::vector<Detection> detections;
    for (int i = 0; i < candidates; ++i) {
        const float raw_score = at(i, 4);
        const float score = raw_score > 1.0f || raw_score < 0.0f ? sigmoid(raw_score) : raw_score;
        if (score < score_threshold) {
            continue;
        }

        Detection detection;
        detection.score = score;
        for (int k = 0; k < kKeypointCount; ++k) {
            const int base = 5 + k * kValuesPerKeypoint;
            float x = at(i, base);
            float y = at(i, base + 1);
            float confidence = at(i, base + 2);
            confidence = confidence > 1.0f || confidence < 0.0f ? sigmoid(confidence) : confidence;

            x = (x - static_cast<float>(letterbox_info.pad_x)) / letterbox_info.scale;
            y = (y - static_cast<float>(letterbox_info.pad_y)) / letterbox_info.scale;
            x = std::clamp(x, 0.0f, static_cast<float>(frame_width - 1));
            y = std::clamp(y, 0.0f, static_cast<float>(frame_height - 1));
            detection.keypoints[k] = {x / static_cast<float>(frame_width), y / static_cast<float>(frame_height), confidence};
        }
        detections.push_back(detection);
    }

    std::sort(detections.begin(), detections.end(), [](const Detection& a, const Detection& b) {
        return a.score > b.score;
    });
    return detections;
}

int choose_visible_side(const Detection& detection) {
    const float left_score =
        detection.keypoints[LeftShoulder].confidence +
        detection.keypoints[LeftHip].confidence +
        detection.keypoints[LeftKnee].confidence +
        detection.keypoints[LeftAnkle].confidence +
        detection.keypoints[LeftWrist].confidence;
    const float right_score =
        detection.keypoints[RightShoulder].confidence +
        detection.keypoints[RightHip].confidence +
        detection.keypoints[RightKnee].confidence +
        detection.keypoints[RightAnkle].confidence +
        detection.keypoints[RightWrist].confidence;
    return left_score >= right_score ? 0 : 1;
}

Keypoint choose_front_ankle(const Detection& detection, const std::string& goal_direction) {
    const Keypoint left = detection.keypoints[LeftAnkle];
    const Keypoint right = detection.keypoints[RightAnkle];
    if (goal_direction == "right") {
        return left.x >= right.x ? left : right;
    }
    return left.x <= right.x ? left : right;
}

Keypoint choose_back_ankle(const Detection& detection, const std::string& goal_direction) {
    const Keypoint front = choose_front_ankle(detection, goal_direction);
    const Keypoint left = detection.keypoints[LeftAnkle];
    const Keypoint right = detection.keypoints[RightAnkle];
    return front.x == left.x && front.y == left.y ? right : left;
}

ExportRow make_export_row(const Detection& detection, int frame_index, double time, const std::string& goal_direction) {
    const bool use_left = choose_visible_side(detection) == 0;

    ExportRow row;
    row.frame = frame_index;
    row.time = time;
    row.shoulder = detection.keypoints[use_left ? LeftShoulder : RightShoulder];
    row.hip = detection.keypoints[use_left ? LeftHip : RightHip];
    row.knee = detection.keypoints[use_left ? LeftKnee : RightKnee];
    row.ankle = detection.keypoints[use_left ? LeftAnkle : RightAnkle];
    row.front_ankle = choose_front_ankle(detection, goal_direction);
    row.back_ankle = choose_back_ankle(detection, goal_direction);
    row.wrist = detection.keypoints[use_left ? LeftWrist : RightWrist];
    return row;
}

void write_header(std::ofstream& out) {
    out << "frame,time,shoulder_x,shoulder_y,hip_x,hip_y,knee_x,knee_y,ankle_x,ankle_y,front_ankle_x,front_ankle_y,back_ankle_x,back_ankle_y,wrist_x,wrist_y\n";
}

void write_row(std::ofstream& out, const ExportRow& row) {
    out << row.frame << ','
        << row.time << ','
        << row.shoulder.x << ',' << row.shoulder.y << ','
        << row.hip.x << ',' << row.hip.y << ','
        << row.knee.x << ',' << row.knee.y << ','
        << row.ankle.x << ',' << row.ankle.y << ','
        << row.front_ankle.x << ',' << row.front_ankle.y << ','
        << row.back_ankle.x << ',' << row.back_ankle.y << ','
        << row.wrist.x << ',' << row.wrist.y << '\n';
}

}  // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 2 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
            print_usage(argv[0]);
            return 0;
        }
        if (argc < 4 || argc > 5) {
            print_usage(argv[0]);
            return 1;
        }

        const std::string video_path = argv[1];
        const std::string model_path = argv[2];
        const std::string output_csv_path = argv[3];
        const std::string goal_direction = argc == 5 ? argv[4] : "right";
        if (!is_goal_direction(goal_direction)) {
            throw std::runtime_error("Goal direction must be 'right' or 'left'");
        }

        cv::VideoCapture capture(video_path);
        if (!capture.isOpened()) {
            throw std::runtime_error("Cannot open video: " + video_path);
        }
        const double fps = capture.get(cv::CAP_PROP_FPS) > 0.0 ? capture.get(cv::CAP_PROP_FPS) : 30.0;

        cv::dnn::Net net = cv::dnn::readNetFromONNX(model_path);
        net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);

        ensure_parent_directory(output_csv_path);
        std::ofstream out(output_csv_path);
        if (!out) {
            throw std::runtime_error("Cannot write landmarks CSV: " + output_csv_path);
        }
        out << std::fixed << std::setprecision(6);
        write_header(out);

        int frame_index = 0;
        int exported = 0;
        cv::Mat frame;
        while (capture.read(frame)) {
            LetterboxInfo info;
            const cv::Mat input_image = letterbox(frame, info);
            const cv::Mat blob = cv::dnn::blobFromImage(input_image, 1.0 / 255.0, cv::Size(kInputSize, kInputSize), cv::Scalar(), true, false);
            net.setInput(blob);
            const cv::Mat output = net.forward();

            const auto detections = decode_yolov8_pose(output, info, frame.cols, frame.rows, 0.25f);
            if (!detections.empty()) {
                const double time = static_cast<double>(frame_index) / fps;
                const ExportRow row = make_export_row(detections.front(), frame_index, time, goal_direction);
                write_row(out, row);
                ++exported;
            }
            ++frame_index;
        }

        if (exported < 2) {
            throw std::runtime_error("Pose model produced fewer than two usable frames");
        }

        std::cout << "Exported " << exported << " landmark frames to " << output_csv_path << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
