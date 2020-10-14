// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include "core/providers/hip/hip_common.h"
#include "core/providers/hip/multi_tensor/common.cuh"

namespace onnxruntime {
namespace rocm {

template <typename TIn, typename TOut>
class ReduceAllL2 final : public RocmKernel {
 public:
  ReduceAllL2(const OpKernelInfo& info) : RocmKernel(info) {}

  Status ComputeInternal(OpKernelContext* context) const override;
};

template <typename TIn, typename TOut>
struct MultiTensorReduceL2 {
  void operator()(ChunkGroup<1> chunk_group, TOut* output);
};

template<typename Tin, typename Tout>
void ScalarSqrt(Tin* input, Tout* output);

}  // namespace rocm
}  // namespace onnxruntime
