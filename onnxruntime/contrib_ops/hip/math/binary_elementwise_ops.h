// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "core/providers/hip/math/binary_elementwise_ops.h"
#include "core/providers/hip/hip_common.h"
#include "core/providers/hip/shared_inc/fast_divmod.h"
#include "core/providers/cpu/tensor/utils.h"

using namespace onnxruntime::hip;

namespace onnxruntime {
namespace contrib {
namespace hip {

// AddGelu fuse Add + Gelu
template <typename T>
class BiasGelu final : public BinaryElementwise<ShouldBroadcast> {
 public:
  BiasGelu(const OpKernelInfo& info) : BinaryElementwise(info) {
  }

  Status ComputeInternal(OpKernelContext* context) const override;
};

}  // namespace hip
}  // namespace contrib
}  // namespace onnxruntime
