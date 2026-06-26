# Hockey Shot Analyzer

真横カメラで撮影したアイスホッケーのリストショットを、骨格ランドマークCSVから評価するC++プロトタイプです。

## 入力CSV

`landmarks.csv` は次の列を持ちます。

```csv
frame,time,shoulder_x,shoulder_y,hip_x,hip_y,knee_x,knee_y,ankle_x,ankle_y,front_ankle_x,front_ankle_y,back_ankle_x,back_ankle_y,wrist_x,wrist_y
```

- 座標は0.0〜1.0の正規化座標を想定します。
- 真横映像なので、肩・腰・膝・足首・手首はカメラ側に見えている片側の点を使います。
- `front_ankle` と `back_ankle` は前足・後ろ足の足首です。
- 重心移動、スタンス幅、手首速度、フォロースルー量は、肩から足首までの高さで正規化します。これにより撮影距離やズームの影響を抑えます。
- CSVヘッダーは上記の列名・順番と一致している必要があります。列が足りない、順番が違う、数値でない値がある場合はエラーになります。

## 出力

- `metrics.csv`: フレームごとの局面、膝角度、前傾角、身体スケール、手首速度など
- `report.json`: 6指標のスコア、総合スコア、リリース推定、実測値、改善コメント
- `timeline.svg`: 手首速度・膝角度・前傾角・シュート局面の簡易グラフ
- `report.html`: スコア、実測値、改善コメント、タイムラインをまとめた閲覧用レポート

## 実行

使い方を確認するには次を実行します。

```bash
./build/hockey_shot_analyzer --help
```

```bash
cmake -S . -B build
cmake --build build
./build/hockey_shot_analyzer samples/landmarks.csv outputs/metrics.csv outputs/report.json right outputs/timeline.svg outputs/report.html
```

この環境のようにCMakeがない場合は、次のように直接コンパイルできます。

```bash
mkdir -p build outputs
clang++ -std=c++17 -Wall -Wextra -Wpedantic src/*.cpp -o build/hockey_shot_analyzer
./build/hockey_shot_analyzer samples/landmarks.csv outputs/metrics.csv outputs/report.json right outputs/timeline.svg outputs/report.html
```

最後の `right` は、画面上でゴールが右にあるという意味です。ゴールが左にある映像では `left` を指定します。省略した場合は `right` として扱います。
`timeline.svg` も省略可能です。指定すると、ブラウザで開ける可視化ファイルが出力されます。
`report.html` を指定すると、スコアや改善コメントを1ページで見られるHTMLレポートが出力されます。
出力先フォルダが存在しない場合は自動で作成されます。

採点基準を外部ファイルで指定する場合は、最後に `config/scoring.conf` を渡します。

```bash
./build/hockey_shot_analyzer samples/landmarks.csv outputs/metrics.csv outputs/report.json right outputs/timeline.svg outputs/report.html config/scoring.conf
```

## 動画から解析する場合

動画を直接解析するには、追加ツール `video_to_landmarks` を使います。これは **OpenCV DNN** と **YOLOv8形式のPose ONNXモデル** を使います。ONNX Runtimeは不要です。

Ubuntuで必要なパッケージ例:

```bash
sudo apt update
sudo apt install build-essential cmake libopencv-dev
```

Ubuntuでは次の補助スクリプトでもビルドできます。

```bash
bash scripts/build_ubuntu.sh
```

ビルド例:

```bash
cmake -S . -B build -DBUILD_VIDEO_PIPELINE=ON
cmake --build build
```

OpenCVが自動検出されない場合は、`OpenCV_DIR` を指定します。

```bash
cmake -S . -B build -DBUILD_VIDEO_PIPELINE=ON -DOpenCV_DIR=/path/to/opencv/cmake
```

実行例:

```bash
bash scripts/analyze_video.sh shot.mp4 yolov8n-pose.onnx outputs/video_run right config/scoring.conf
```

処理の流れ:

```text
動画
↓ video_to_landmarks
landmarks.csv
↓ hockey_shot_analyzer
metrics.csv / report.json / timeline.svg / report.html
```

注意:

- モデルはCOCO 17点キーポイントのYOLOv8 Pose系ONNXを想定しています。
- OpenCV DNNで読めるONNXモデルが必要です。まずは `yolov8n-pose.onnx` のような軽量モデルから試すのがおすすめです。
- 真横映像を前提に、左右のうち信頼度が高い側の肩・腰・膝・足首・手首を使います。
- 前足と後ろ足は、指定したゴール方向に対する足首のx座標で推定します。

## ファイル構成

- `src/Types.h`: 解析で使うデータ構造
- `src/CsvReader.*`: `landmarks.csv` の読み込み
- `src/MetricsCalculator.*`: 膝角度、前傾角、身体スケール、手首速度などの計算
- `src/ShotAnalyzer.*`: リリース推定、スコアリング、改善コメント生成
- `src/ScoringConfig.*`: 採点基準ファイルの読み込み
- `src/ReportWriter.*`: `metrics.csv` と `report.json` の出力
- `src/TimelineSvgWriter.*`: `timeline.svg` の出力
- `src/HtmlReportWriter.*`: `report.html` の出力
- `src/video/VideoPoseToLandmarks.cpp`: 動画とPose ONNXモデルから `landmarks.csv` を作る追加ツール
- `src/main.cpp`: コマンドライン引数の処理と全体の実行フロー
- `scripts/run_smoke_tests.sh`: ビルドと基本動作を確認するスモークテスト
- `scripts/build_ubuntu.sh`: Ubuntu向けにOpenCV動画パイプラインまでビルドする補助スクリプト
- `scripts/analyze_video.sh`: 動画からHTMLレポートまで一括実行する補助スクリプト

## テスト

基本動作をまとめて確認するには、次を実行します。

```bash
bash scripts/run_smoke_tests.sh
```

このテストでは、右向き解析、左向き解析、出力ファイル生成、CSVヘッダー検査を確認します。

## 採点基準の調整

`config/scoring.conf` で理想範囲、許容幅、総合スコアの重みを調整できます。

例:

```conf
knee.low = 120
knee.high = 145
knee.tolerance = 35

weights.knee = 0.20
weights.weight_transfer = 0.25
```

`low`〜`high` の範囲内なら100点です。範囲から外れると `tolerance` に応じて減点されます。

## 評価指標

- 膝屈曲角
- 重心の前後移動
- 前後スタンス幅
- 上半身前傾角
- 手首速度ピーク
- フォロースルー方向・高さ

## レポート内容

`report.json` には次の情報が入ります。

- `total_score`: 重み付き総合スコア
- `goal_direction`: 解析時に指定したゴール方向
- `release`: 手首速度ピークから推定したリリース付近のフレーム
- `scores`: 6項目の0〜100点スコア
- `measured_values`: 採点の根拠になった実測値
- `advice`: スコアが低い項目への改善コメント

## シュート局面

`metrics.csv` の `phase` 列には、次のいずれかが入ります。

- `setup`: 構え
- `loading`: 体重移動やタメ
- `release`: 手首速度ピークから推定したリリース付近
- `follow_through`: リリース後
