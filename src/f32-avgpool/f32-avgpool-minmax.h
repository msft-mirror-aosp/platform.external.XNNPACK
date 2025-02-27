// Copyright 2024 Google LLC
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

// arch_flags, ukernel, channel_tile, primary_tile, datatype, params_type, init_params

#if XNN_ARCH_ARM || XNN_ARCH_ARM64
XNN_UKERNEL(0, xnn_f32_avgpool_minmax_ukernel_9p__neon_u4, 4, 9, float, struct xnn_f32_scaleminmax_params, xnn_init_f32_scaleminmax_scalar_params)
#endif  // XNN_ARCH_ARM || XNN_ARCH_ARM64

#if XNN_ARCH_X86 || XNN_ARCH_X86_64
XNN_UKERNEL(0, xnn_f32_avgpool_minmax_ukernel_9p__sse2_u4, 4, 9, float, struct xnn_f32_scaleminmax_params, xnn_init_f32_scaleminmax_scalar_params)
#endif  // XNN_ARCH_X86 || XNN_ARCH_X86_64

#if XNN_ARCH_WASMSIMD || XNN_ARCH_WASMRELAXEDSIMD
XNN_UKERNEL(0, xnn_f32_avgpool_minmax_ukernel_9p__wasmsimd_u4, 4, 9, float, struct xnn_f32_scaleminmax_params, xnn_init_f32_scaleminmax_scalar_params)
#endif  // XNN_ARCH_WASMSIMD || XNN_ARCH_WASMRELAXEDSIMD


XNN_UKERNEL(0, xnn_f32_avgpool_minmax_ukernel_9p__scalar_u1, 1, 9, float, struct xnn_f32_scaleminmax_params, xnn_init_f32_scaleminmax_scalar_params)
