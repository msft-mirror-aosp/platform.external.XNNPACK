// Copyright (c) Facebook, Inc. and its affiliates.
// All rights reserved.
//
// Copyright 2019 Google LLC
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include <gtest/gtest.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <random>
#include <vector>

#include <fp16.h>

#include <xnnpack.h>


class SigmoidOperatorTester {
 public:
  inline SigmoidOperatorTester& channels(size_t channels) {
    assert(channels != 0);
    this->channels_ = channels;
    return *this;
  }

  inline size_t channels() const {
    return this->channels_;
  }

  inline SigmoidOperatorTester& input_stride(size_t input_stride) {
    assert(input_stride != 0);
    this->input_stride_ = input_stride;
    return *this;
  }

  inline size_t input_stride() const {
    if (this->input_stride_ == 0) {
      return this->channels_;
    } else {
      assert(this->input_stride_ >= this->channels_);
      return this->input_stride_;
    }
  }

  inline SigmoidOperatorTester& output_stride(size_t output_stride) {
    assert(output_stride != 0);
    this->output_stride_ = output_stride;
    return *this;
  }

  inline size_t output_stride() const {
    if (this->output_stride_ == 0) {
      return this->channels_;
    } else {
      assert(this->output_stride_ >= this->channels_);
      return this->output_stride_;
    }
  }

  inline SigmoidOperatorTester& batch_size(size_t batch_size) {
    assert(batch_size != 0);
    this->batch_size_ = batch_size;
    return *this;
  }

  inline size_t batch_size() const {
    return this->batch_size_;
  }

  inline SigmoidOperatorTester& input_scale(float input_scale) {
    assert(input_scale > 0.0f);
    assert(std::isnormal(input_scale));
    this->input_scale_ = input_scale;
    return *this;
  }

  inline float input_scale() const {
    return this->input_scale_;
  }

  inline SigmoidOperatorTester& input_zero_point(uint8_t input_zero_point) {
    this->input_zero_point_ = input_zero_point;
    return *this;
  }

  inline uint8_t input_zero_point() const {
    return this->input_zero_point_;
  }

  inline float output_scale() const {
    return 1.0f / 256.0f;
  }

  inline uint8_t output_zero_point() const {
    return 0;
  }

  inline SigmoidOperatorTester& qmin(uint8_t qmin) {
    this->qmin_ = qmin;
    return *this;
  }

  inline uint8_t qmin() const {
    return this->qmin_;
  }

  inline SigmoidOperatorTester& qmax(uint8_t qmax) {
    this->qmax_ = qmax;
    return *this;
  }

  inline uint8_t qmax() const {
    return this->qmax_;
  }

  inline SigmoidOperatorTester& iterations(size_t iterations) {
    this->iterations_ = iterations;
    return *this;
  }

  inline size_t iterations() const {
    return this->iterations_;
  }

  void TestF16() const {
    std::random_device random_device;
    auto rng = std::mt19937(random_device());
    std::uniform_real_distribution<float> f32dist(-25.0f, 25.0f);

    std::vector<uint16_t> input((batch_size() - 1) * input_stride() + channels() + XNN_EXTRA_BYTES / sizeof(uint16_t));
    std::vector<uint16_t> output((batch_size() - 1) * output_stride() + channels());
    std::vector<float> output_ref(batch_size() * channels());
    for (size_t iteration = 0; iteration < iterations(); iteration++) {
      std::generate(input.begin(), input.end(), [&]() { return fp16_ieee_from_fp32_value(f32dist(rng)); });
      std::fill(output.begin(), output.end(), UINT16_C(0x7E00) /* NaN */);

      // Compute reference results.
      for (size_t i = 0; i < batch_size(); i++) {
        for (size_t c = 0; c < channels(); c++) {
          const float x = fp16_ieee_to_fp32_value(input[i * input_stride() + c]);
          const float exp_x = std::exp(x);
          const float sigmoid_x = exp_x / (1.0 + exp_x);
          output_ref[i * channels() + c] = sigmoid_x;
        }
      }

      // Create, setup, run, and destroy Sigmoid operator.
      ASSERT_EQ(xnn_status_success, xnn_initialize(nullptr /* allocator */));
      xnn_operator_t sigmoid_op = nullptr;

      const xnn_status status = xnn_create_sigmoid_nc_f16(
          channels(), input_stride(), output_stride(),
          0, &sigmoid_op);
      if (status == xnn_status_unsupported_hardware) {
        GTEST_SKIP();
      }
      ASSERT_EQ(xnn_status_success, status);
      ASSERT_NE(nullptr, sigmoid_op);

      // Smart pointer to automatically delete sigmoid_op.
      std::unique_ptr<xnn_operator, decltype(&xnn_delete_operator)> auto_sigmoid_op(sigmoid_op, xnn_delete_operator);

      ASSERT_EQ(xnn_status_success,
        xnn_setup_sigmoid_nc_f16(
          sigmoid_op,
          batch_size(),
          input.data(), output.data(),
          nullptr /* thread pool */));

      ASSERT_EQ(xnn_status_success,
        xnn_run_operator(sigmoid_op, nullptr /* thread pool */));

      // Verify results.
      for (size_t i = 0; i < batch_size(); i++) {
        for (size_t c = 0; c < channels(); c++) {
          ASSERT_NEAR(
              fp16_ieee_to_fp32_value(output[i * output_stride() + c]),
              output_ref[i * channels() + c],
              std::max(1.0e-4f, std::abs(output_ref[i * channels() + c]) * 5.0e-3f));
        }
      }
    }
  }

  void TestF32() const {
    std::random_device random_device;
    auto rng = std::mt19937(random_device());
    std::uniform_real_distribution<float> f32dist(-25.0f, 25.0f);

    std::vector<float> input((batch_size() - 1) * input_stride() + channels() + XNN_EXTRA_BYTES / sizeof(float));
    std::vector<float> output((batch_size() - 1) * output_stride() + channels());
    std::vector<double> output_ref(batch_size() * channels());
    for (size_t iteration = 0; iteration < iterations(); iteration++) {
      std::generate(input.begin(), input.end(), [&]() { return f32dist(rng); });
      std::fill(output.begin(), output.end(), std::nanf(""));

      // Compute reference results.
      for (size_t i = 0; i < batch_size(); i++) {
        for (size_t c = 0; c < channels(); c++) {
          const double x = input[i * input_stride() + c];
          const double exp_x = std::exp(x);
          const double sigmoid_x = exp_x / (1.0 + exp_x);
          output_ref[i * channels() + c] = sigmoid_x;
        }
      }

      // Create, setup, run, and destroy Sigmoid operator.
      ASSERT_EQ(xnn_status_success, xnn_initialize(nullptr /* allocator */));
      xnn_operator_t sigmoid_op = nullptr;

      xnn_status status = xnn_create_sigmoid_nc_f32(
          channels(), input_stride(), output_stride(),
          0, &sigmoid_op);
      ASSERT_EQ(xnn_status_success, status);
      ASSERT_NE(nullptr, sigmoid_op);

      // Smart pointer to automatically delete sigmoid_op.
      std::unique_ptr<xnn_operator, decltype(&xnn_delete_operator)> auto_sigmoid_op(sigmoid_op, xnn_delete_operator);

      ASSERT_EQ(xnn_status_success,
        xnn_setup_sigmoid_nc_f32(
          sigmoid_op,
          batch_size(),
          input.data(), output.data(),
          nullptr /* thread pool */));

      ASSERT_EQ(xnn_status_success,
        xnn_run_operator(sigmoid_op, nullptr /* thread pool */));

      // Verify results.
      for (size_t i = 0; i < batch_size(); i++) {
        for (size_t c = 0; c < channels(); c++) {
          ASSERT_NEAR(
            output[i * output_stride() + c],
            output_ref[i * channels() + c],
            5.0e-6);
        }
      }
    }
  }

  void TestQS8() const {
    std::random_device random_device;
    auto rng = std::mt19937(random_device());
    std::uniform_int_distribution<int32_t> i8dist(
      std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max());

    std::vector<int8_t> input((batch_size() - 1) * input_stride() + channels() + XNN_EXTRA_BYTES / sizeof(int8_t));
    std::vector<int8_t> output((batch_size() - 1) * output_stride() + channels());
    std::vector<float> output_ref(batch_size() * channels());
    for (size_t iteration = 0; iteration < iterations(); iteration++) {
      std::generate(input.begin(), input.end(), [&]() { return i8dist(rng); });
      std::fill(output.begin(), output.end(), INT8_C(0xA5));

      // Compute reference results.
      for (size_t i = 0; i < batch_size(); i++) {
        for (size_t c = 0; c < channels(); c++) {
          const float x = input_scale() *
            (int32_t(input[i * input_stride() + c]) - int32_t(input_zero_point() - 0x80));
          const float sigmoid_x = 1.0f / (1.0f + std::exp(-x));
          const float scaled_sigmoid_x = sigmoid_x / output_scale();
          float y = scaled_sigmoid_x;
          y = std::min<float>(y, int32_t(qmax() - 0x80) - int32_t(output_zero_point() - 0x80));
          y = std::max<float>(y, int32_t(qmin() - 0x80) - int32_t(output_zero_point() - 0x80));
          output_ref[i * channels() + c] = y + int32_t(output_zero_point() - 0x80);
        }
      }

      // Create, setup, run, and destroy Sigmoid operator.
      ASSERT_EQ(xnn_status_success, xnn_initialize(nullptr /* allocator */));
      xnn_operator_t sigmoid_op = nullptr;

      ASSERT_EQ(xnn_status_success,
        xnn_create_sigmoid_nc_qs8(
          channels(), input_stride(), output_stride(),
          int8_t(input_zero_point() - 0x80), input_scale(),
          int8_t(output_zero_point() - 0x80), output_scale(),
          int8_t(qmin() - 0x80), int8_t(qmax() - 0x80),
          0, &sigmoid_op));
      ASSERT_NE(nullptr, sigmoid_op);

      // Smart pointer to automatically delete sigmoid_op.
      std::unique_ptr<xnn_operator, decltype(&xnn_delete_operator)> auto_sigmoid_op(sigmoid_op, xnn_delete_operator);

      ASSERT_EQ(xnn_status_success,
        xnn_setup_sigmoid_nc_qs8(
          sigmoid_op,
          batch_size(),
          input.data(), output.data(),
          nullptr /* thread pool */));

      ASSERT_EQ(xnn_status_success,
        xnn_run_operator(sigmoid_op, nullptr /* thread pool */));

      // Verify results.
      for (size_t i = 0; i < batch_size(); i++) {
        for (size_t c = 0; c < channels(); c++) {
          ASSERT_NEAR(float(int32_t(output[i * output_stride() + c])), output_ref[i * channels() + c], 0.6f);
        }
      }
    }
  }

  void TestQU8() const {
    std::random_device random_device;
    auto rng = std::mt19937(random_device());
    std::uniform_int_distribution<int32_t> u8dist(
      std::numeric_limits<uint8_t>::min(), std::numeric_limits<uint8_t>::max());

    std::vector<uint8_t> input((batch_size() - 1) * input_stride() + channels() + XNN_EXTRA_BYTES / sizeof(uint8_t));
    std::vector<uint8_t> output((batch_size() - 1) * output_stride() + channels());
    std::vector<float> output_ref(batch_size() * channels());
    for (size_t iteration = 0; iteration < iterations(); iteration++) {
      std::generate(input.begin(), input.end(), [&]() { return u8dist(rng); });
      std::fill(output.begin(), output.end(), UINT8_C(0xA5));

      // Compute reference results.
      for (size_t i = 0; i < batch_size(); i++) {
        for (size_t c = 0; c < channels(); c++) {
          const float x = input_scale() *
            (int32_t(input[i * input_stride() + c]) - int32_t(input_zero_point()));
          const float sigmoid_x = 1.0f / (1.0f + std::exp(-x));
          const float scaled_sigmoid_x = sigmoid_x / output_scale();
          float y = scaled_sigmoid_x;
          y = std::min<float>(y, int32_t(qmax()) - int32_t(output_zero_point()));
          y = std::max<float>(y, int32_t(qmin()) - int32_t(output_zero_point()));
          output_ref[i * channels() + c] = y + int32_t(output_zero_point());
        }
      }

      // Create, setup, run, and destroy Sigmoid operator.
      ASSERT_EQ(xnn_status_success, xnn_initialize(nullptr /* allocator */));
      xnn_operator_t sigmoid_op = nullptr;

      ASSERT_EQ(xnn_status_success,
        xnn_create_sigmoid_nc_qu8(
          channels(), input_stride(), output_stride(),
          input_zero_point(), input_scale(),
          output_zero_point(), output_scale(),
          qmin(), qmax(),
          0, &sigmoid_op));
      ASSERT_NE(nullptr, sigmoid_op);

      // Smart pointer to automatically delete sigmoid_op.
      std::unique_ptr<xnn_operator, decltype(&xnn_delete_operator)> auto_sigmoid_op(sigmoid_op, xnn_delete_operator);

      ASSERT_EQ(xnn_status_success,
        xnn_setup_sigmoid_nc_qu8(
          sigmoid_op,
          batch_size(),
          input.data(), output.data(),
          nullptr /* thread pool */));

      ASSERT_EQ(xnn_status_success,
        xnn_run_operator(sigmoid_op, nullptr /* thread pool */));

      // Verify results.
      for (size_t i = 0; i < batch_size(); i++) {
        for (size_t c = 0; c < channels(); c++) {
          ASSERT_NEAR(float(int32_t(output[i * output_stride() + c])), output_ref[i * channels() + c], 0.6f);
        }
      }
    }
  }

 private:
  size_t batch_size_{1};
  size_t channels_{1};
  size_t input_stride_{0};
  size_t output_stride_{0};
  float input_scale_{0.75f};
  uint8_t input_zero_point_{121};
  uint8_t qmin_{0};
  uint8_t qmax_{255};
  size_t iterations_{15};
};
