// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "ocos.h"
#include <dlib/matrix.h>

struct StftNormal {
  StftNormal() = default;

  OrtStatusPtr OnModelAttach(const OrtApi& api, const OrtKernelInfo& info) {
    return OrtW::GetOpAttribute(info, "onesided", onesided_);
  }

  OrtxStatus Compute(const ortc::Tensor<float>& pcm, int64_t n_fft, int64_t hop_length,
                     const ortc::Span<float>& win, int64_t frame_length, ortc::Tensor<float>& output0) const {
    auto X = pcm.Data();
    auto window = win.data_;
    auto dimensions = pcm.Shape();
    auto win_length = win.size();

    if (dimensions.size() < 2 || pcm.NumberOfElement() != dimensions[1]) {
      return {kOrtxErrorInvalidArgument, "[Stft] Only batch == 1 tensor supported."};
    }
    if (frame_length != n_fft) {
      return {kOrtxErrorInvalidArgument, "[Stft] Only support size of FFT equals the frame length."};
    }

    dlib::matrix<float> dm_x = dlib::mat(X, 1, dimensions[1]);
    dlib::matrix<float> fft_win = dlib::mat(window, 1, win_length);

    auto m_stft =
      dlib::stft(dm_x, [&fft_win](size_t x, size_t len) { return fft_win(0, x); }, n_fft, win_length, hop_length);

    if (onesided_) {
      m_stft = dlib::subm(m_stft, 0, 0, m_stft.nr(), (m_stft.nc() >> 1) + 1);
    }

    dlib::matrix<float> result = dlib::norm(m_stft);
    result = dlib::trans(result);
    std::vector<int64_t> outdim{1, result.nr(), result.nc()};
    auto result_size = result.size();
    auto out0 = output0.Allocate(outdim);
    memcpy(out0, result.steal_memory().get(), result_size * sizeof(float));

    return {};
  }

 private:
  int64_t onesided_{1};
};
