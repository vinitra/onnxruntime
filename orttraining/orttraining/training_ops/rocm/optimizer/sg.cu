#include "hip/hip_runtime.h"
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "core/providers/rocm/hip_common.h"
#include "core/providers/rocm/cu_inc/common.cuh"
#include "core/providers/rocm/atomic/common.cuh"
#include "sg.h"

namespace onnxruntime {
namespace rocm {

template <typename T>
__global__ void _SGDOptimizer(
    const T* eta,
    const T* weights,
    const T* gradients,
    T* weights_out,
    T* gradients_out,
    HIP_LONG N) {
  CALCULATE_ELEMENTWISE_INDEX_OR_EXIT(id, N);

  const T delta = -(*eta) * gradients[id];

  if (gradients_out) {
    gradients_out[id] = delta;
  }
  if (weights_out) {
    weights_out[id] = weights[id] + delta;
  }
}

template <typename T>
void SGDOptimizerImpl(
    const T* eta,
    const T* weights,
    const T* gradients,
    T* weights_out,
    T* gradients_out,
    size_t count) {
  int blocksPerGrid = (int)(ceil(static_cast<float>(count) / GridDim::maxThreadsPerBlock));
  HIP_LONG N = static_cast<HIP_LONG>(count);
  hipLaunchKernelGGL(HIP_KERNEL_NAME(_SGDOptimizer<T>), dim3(blocksPerGrid), dim3(GridDim::maxThreadsPerBlock), 0, 0, 
      eta,
      weights,
      gradients,
      weights_out,
      gradients_out,
      N);
}

#define SPECIALIZED_IMPL__SGDOptimizerImpl(T) \
  template void SGDOptimizerImpl(             \
      const T* eta,                           \
      const T* weights,                       \
      const T* gradients,                     \
      T* weights_out,                         \
      T* gradients_out,                       \
      size_t count);

SPECIALIZED_IMPL__SGDOptimizerImpl(float)

}  // namespace rocm
}  // namespace onnxruntime
