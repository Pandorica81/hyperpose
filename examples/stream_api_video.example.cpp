#include "utils.hpp"
#include <gflags/gflags.h>
#include <opencv2/opencv.hpp>
#include <openpose_plus/openpose_plus.hpp>

DEFINE_string(model_file, "../data/models/hao28-600000-256x384.uff",
    "Path to uff model.");
DEFINE_string(input_name, "image", "The input node name of your uff model file.");
DEFINE_string(output_name_list, "outputs/conf,outputs/paf", "The output node names(maybe more than one) of your uff model file.");

DEFINE_int32(input_width, 384, "Width of input image.");
DEFINE_int32(input_height, 256, "Height of input image.");
DEFINE_int32(max_batch_size, 8, "Max batch size for inference engine to execute.");

DEFINE_bool(original_resolution, false, "Use the original image size as the output image size. (otherwise, use the network input size)");
DEFINE_string(input_video, "../data/media/video.avi", "Video to be processed.");
DEFINE_string(output_video, "output_video.avi", "The name of output video.");
DEFINE_bool(logging, false, "Print the logging information or not.");

int main(int argc, char** argv)
{
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    namespace pp = poseplus;

    if (FLAGS_logging)
        pp::enable_logging();

    auto capture = cv::VideoCapture(FLAGS_input_video);

    auto writer = cv::VideoWriter(
        FLAGS_output_video,
        cv::VideoWriter::fourcc('M', 'J', 'P', 'G'),
        capture.get(cv::CAP_PROP_FPS),
        cv::Size(capture.get(cv::CAP_PROP_FRAME_WIDTH), capture.get(cv::CAP_PROP_FRAME_HEIGHT)));

    // Basic information about videos.
    poseplus_log() << "Input video name: " << FLAGS_input_video << std::endl;
    poseplus_log() << "Output video name: " << FLAGS_output_video << std::endl;
    poseplus_log() << "Input Frame: Size@" << cv::Size(capture.get(cv::CAP_PROP_FRAME_WIDTH), capture.get(cv::CAP_PROP_FRAME_HEIGHT)) << std::endl;
    poseplus_log() << "Output Frame: Size@" << cv::Size(writer.get(cv::CAP_PROP_FRAME_WIDTH), writer.get(cv::CAP_PROP_FRAME_HEIGHT)) << std::endl;
    poseplus_log() << "Count@" << capture.get(cv::CAP_PROP_FRAME_COUNT) << std::endl;

    // Checks.
    if (!capture.isOpened()) {
        poseplus_log() << "Video: " << FLAGS_input_video << " cannot be opened\n";
        std::exit(-1);
    }

    auto engine = [&] {
        using namespace pp::dnn;
        constexpr std::string_view onnx_suffix = ".onnx";
        constexpr std::string_view uff_suffix = ".uff";

        if (std::equal(onnx_suffix.crbegin(), onnx_suffix.crend(), FLAGS_model_file.crbegin()))
            return tensorrt(onnx{ FLAGS_model_file }, { FLAGS_input_width, FLAGS_input_height }, FLAGS_max_batch_size);

        if (std::equal(uff_suffix.crbegin(), uff_suffix.crend(), FLAGS_model_file.crbegin()))
            return tensorrt(
                uff{ FLAGS_model_file, FLAGS_input_name, split(FLAGS_output_name_list, ',') },
                { FLAGS_input_width, FLAGS_input_height },
                FLAGS_max_batch_size);

        poseplus_log() << "Your model file's suffix is not [.onnx | .uff]. Your model file path: " << FLAGS_model_file;
        std::exit(1);
    }();

    pp::parser::paf parser{};

    auto stream = pp::make_stream(engine, parser, FLAGS_original_resolution);

    stream.add_monitor(1000);

    size_t total_frames = capture.get(cv::CAP_PROP_FRAME_COUNT);

    using clk_t = std::chrono::high_resolution_clock;
    auto beg = clk_t::now();

    stream.async() << capture;
    stream.sync() >> writer;

    auto millis = std::chrono::duration<double, std::milli>(clk_t::now() - beg).count();

    std::cout << total_frames << " images got processed in " << millis << " ms, FPS = "
              << 1000. * total_frames / millis << '\n';
}