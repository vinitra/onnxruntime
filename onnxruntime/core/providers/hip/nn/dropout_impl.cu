#include "hip/hip_runtime.h"
/**
* Copyright (c) 2016-present, Facebook, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

/* Modifications Copyright (c) Microsoft. */

#include "core/providers/hip/cu_inc/common.cuh"
#include "core/providers/hip/nn/dropout_impl.h"
#include <hiprand_kernel.h>
#include <algorithm>

namespace onnxruntime {
namespace hip {

constexpr int UNROLL = 4;

template <typename T>
__global__ void DropoutKernel(
    const int64_t N,
    const float ratio,
    const std::pair<uint64_t, uint64_t> seeds,
    const T* X_data,
    T* Y_data,
    bool* mask_data) {
  const float p = 1.0f - ratio;
  const float scale = 1.0f / p;

  HIP_LONG idx = blockDim.x * blockIdx.x + threadIdx.x;
  HIP_LONG step_size = gridDim.x * blockDim.x * UNROLL;
  HIP_LONG rounded_size = ((N - 1) / step_size + 1) * step_size;

  hiprandStatePhilox4_32_10_t state;
  hiprand_init(seeds.first, idx, seeds.second, &state);

  // We ensure every thread generates the same number of random numbers (by rounding
  // up the size) and at the same timestep (by syncing threads).
  // From HIP hiprand documentation:
  //   The Philox_4x32_10 algorithm is closely tied to the thread and block count.
  //   Each thread computes 4 random numbers in the same time thus the most efficient
  //   use of Philox_4x32_10 is to generate a multiple of 4 times number of threads.
  for (HIP_LONG id = idx; id < rounded_size; id += step_size) {
    float4 rand = hiprand_uniform4(&state);
  
  #pragma unroll
    for (HIP_LONG i = 0; i < UNROLL; i++) {
      HIP_LONG li = id + gridDim.x * blockDim.x * i;
      if (li < N) {
        mask_data[li] = (&rand.x)[i] < p;
        Y_data[li] = T(float(X_data[li]) * mask_data[li] * scale);
      }
    }

    __syncthreads();
  }
}

template <typename T>
void DropoutKernelImpl(
    const hipDeviceProp_t& prop,
    const int64_t N,
    const float ratio,
    PhiloxGenerator& generator,
    const T* X_data,
    T* Y_data,
    bool* mask_data) {
  const int block_size = 256;
  const int blocks_per_sm = prop.maxThreadsPerMultiProcessor / block_size;
  const int grid_size = std::min(prop.multiProcessorCount * blocks_per_sm, static_cast<int>(CeilDiv(N, block_size * UNROLL)));

  // Compute the number of random numbers generated by each thread, and increment philox generator offset by that amount.
  const uint64_t counter_offset = static_cast<uint64_t>(((N - 1) / (block_size * grid_size * UNROLL) + 1) * UNROLL);
  auto seeds = generator.NextPhiloxSeeds(counter_offset);

  hipLaunchKernelGGL(HIP_KERNEL_NAME(DropoutKernel<T>), dim3(grid_size), dim3(block_size), 0, 0, N, ratio, seeds, X_data, Y_data, mask_data);
}

#define SPECIALIZED_DROPOUT_IMPL(T) \
  template void DropoutKernelImpl(  \
      const hipDeviceProp_t& prop,   \
      const int64_t N,              \
      const float ratio,            \
      PhiloxGenerator& generator,   \
      const T* X_data,              \
      T* Y_data,                    \
      bool* mask_data);

SPECIALIZED_DROPOUT_IMPL(float)
SPECIALIZED_DROPOUT_IMPL(double)
SPECIALIZED_DROPOUT_IMPL(half)

}  // namespace hip
}  // namespace onnxruntime
