#pragma once

#include <future>
#include <NvInferRuntimeCommon.h>
#include <NvInferRuntime.h>

#include <ttl/cuda_tensor>

#include "../../utility/data.hpp"


namespace sp {

namespace dnn {

template <typename T> struct destroy_deleter {
    void operator()(T *ptr) { ptr->destroy(); }
};

template <typename T>
using destroy_ptr = std::unique_ptr<T, destroy_deleter<T>>;

class tensorrt
{
public:
    explicit tensorrt(const std::string &model_path, cv::Size input_size, const std::string& input_name,
                      const std::vector<std::string> output_names,
                      int max_batch_size = 8, nvinfer1::DataType dtype = nvinfer1::DataType::kFLOAT, double factor = 1., bool flip_rgb = true);

    ~tensorrt();

    inline int max_batch_size() noexcept {
        return m_max_batch_size;
    }

    inline cv::Size input_size() noexcept {
        return m_inp_size;
    }

    inline cv::Size feature_map_size() noexcept  {
        return m_fea_size;
    }

    std::future<inference_result_t> async_inference(const std::vector<cv::Mat>&);

    inference_result_t sync_inference(const std::vector<cv::Mat>&);

private:
    const cv::Size m_inp_size;  // w, h
    const cv::Size m_fea_size;
    const int m_max_batch_size;
    const double m_factor;
    const bool m_flip_rgb;

    // Cuda related.
    destroy_ptr<nvinfer1::ICudaEngine> m_engine;
    using cuda_buffer_t = ttl::cuda_tensor<char, 2>;  // [batch_size, data_size]
    std::vector<cuda_buffer_t> m_cuda_buffers;
};

}

}