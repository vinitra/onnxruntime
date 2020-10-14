// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

namespace onnxruntime {
namespace contrib {
namespace rocm {

template <typename T>
bool LaunchFastGeluKernel(const hipDeviceProp_t& prop, hipStream_t stream, int input_length, int bias_length, const T* input, const T* bias, T* output);

}  // namespace rocm
}  // namespace contrib
}  // namespace onnxruntime
