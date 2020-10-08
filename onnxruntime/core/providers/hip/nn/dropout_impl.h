// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "core/framework/random_generator.h"

namespace onnxruntime {
namespace rocm {

template <typename T>
void DropoutKernelImpl(
  const hipDeviceProp_t& prop,
  const int64_t N,
  const float ratio,
  PhiloxGenerator& generator,
  const T* X_data,
  T* Y_data,
  bool* mask_data);

}  // namespace rocm
}  // namespace onnxruntime
