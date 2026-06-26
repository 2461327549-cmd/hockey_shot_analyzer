cmake_minimum_required(VERSION 3.16)
project(hockey_shot_analyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(hockey_shot_analyzer
    src/CsvReader.cpp
    src/HtmlReportWriter.cpp
    src/main.cpp
    src/MathUtils.cpp
    src/MetricsCalculator.cpp
    src/ReportWriter.cpp
    src/ScoringConfig.cpp
    src/ShotAnalyzer.cpp
    src/TimelineSvgWriter.cpp
)

target_compile_options(hockey_shot_analyzer PRIVATE
    $<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-Wall -Wextra -Wpedantic>
)

option(BUILD_VIDEO_PIPELINE "Build video-to-landmarks tool with OpenCV DNN" OFF)

if(BUILD_VIDEO_PIPELINE)
    find_package(OpenCV REQUIRED)

    add_executable(video_to_landmarks
        src/video/VideoPoseToLandmarks.cpp
    )

    target_include_directories(video_to_landmarks PRIVATE
        ${OpenCV_INCLUDE_DIRS}
    )

    target_link_libraries(video_to_landmarks PRIVATE
        ${OpenCV_LIBS}
    )

    target_compile_options(video_to_landmarks PRIVATE
        $<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-Wall -Wextra -Wpedantic>
    )
endif()
