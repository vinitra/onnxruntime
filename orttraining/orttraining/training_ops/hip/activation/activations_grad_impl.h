// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include "core/providers/hip/activation/activations_impl.h"

namespace onnxruntime {
namespace hip {

typedef onnxruntime::hip::CtxNull CtxGeluGrad;
typedef onnxruntime::hip::CtxNull CtxFastGeluGrad;
typedef onnxruntime::hip::CtxNull CtxReluGrad;

#define ACTIVATION_GRAD_OPS() \
  ACTIVATION_GRAD_OP_NAME(GeluGrad) \
  ACTIVATION_GRAD_OP_NAME(FastGeluGrad) \
  ACTIVATION_GRAD_OP_NAME(ReluGrad)

#define BINARY_ELEMENTWISE_IMPL_DECLARATION(name) \
  template <typename T>                           \
  void Impl_##name(const T* lhs_data,             \
                   const T* rhs_data,             \
                   T* output_data,                \
                   const Ctx##name* func_ctx,     \
                   size_t count)

#define ACTIVATION_GRAD_OP_NAME(name) BINARY_ELEMENTWISE_IMPL_DECLARATION(name);
ACTIVATION_GRAD_OPS()
#undef ACTIVATION_GRAD_OP_NAME

}  // namespace hip
}  // namespace onnxruntime
