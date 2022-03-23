// Auto-generated file. Do not edit!
//   Template: src/f32-qs8-vcvt/scalar-lrintf.c.in
//   Generator: tools/xngen
//
// Copyright 2021 Google LLC
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

#include <assert.h>
#include <math.h>

#include <xnnpack/common.h>
#include <xnnpack/math.h>
#include <xnnpack/vcvt.h>


void xnn_f32_qs8_vcvt_ukernel__scalar_lrintf_x3(
    size_t n,
    const float* x,
    int8_t* y,
    const union xnn_f32_qs8_cvt_params params[restrict XNN_MIN_ELEMENTS(1)])
{
  assert(n != 0);
  assert(n % sizeof(float) == 0);
  assert(x != NULL);
  assert(y != NULL);

  const float vscale = params->scalar_lrintf.scale;
  const float voutput_min_less_zero_point = params->scalar_lrintf.output_min_less_zero_point;
  const float voutput_max_less_zero_point = params->scalar_lrintf.output_max_less_zero_point;
  const int32_t voutput_zero_point = params->scalar_lrintf.output_zero_point;

  for (; n >= 3 * sizeof(float); n -= 3 * sizeof(float)) {
    float vx0 = x[0];
    float vx1 = x[1];
    float vx2 = x[2];
    x += 3;

    vx0 *= vscale;
    vx1 *= vscale;
    vx2 *= vscale;

    vx0 = math_max_f32(vx0, voutput_min_less_zero_point);
    vx1 = math_max_f32(vx1, voutput_min_less_zero_point);
    vx2 = math_max_f32(vx2, voutput_min_less_zero_point);

    vx0 = math_min_f32(vx0, voutput_max_less_zero_point);
    vx1 = math_min_f32(vx1, voutput_max_less_zero_point);
    vx2 = math_min_f32(vx2, voutput_max_less_zero_point);

    int32_t vy0 = (int32_t) lrintf(vx0);
    int32_t vy1 = (int32_t) lrintf(vx1);
    int32_t vy2 = (int32_t) lrintf(vx2);

    vy0 += voutput_zero_point;
    vy1 += voutput_zero_point;
    vy2 += voutput_zero_point;

    y[0] = (int8_t) vy0;
    y[1] = (int8_t) vy1;
    y[2] = (int8_t) vy2;
    y += 3;
  }
  if XNN_UNLIKELY(n != 0) {
    do {
      float vx = *x++;
      vx *= vscale;
      vx = math_max_f32(vx, voutput_min_less_zero_point);
      vx = math_min_f32(vx, voutput_max_less_zero_point);

      int32_t vy = (int32_t) lrintf(vx);
      vy += voutput_zero_point;

      *y++ = (int8_t) vy;

      n -= sizeof(float);
    } while (n != 0);
  }
}
