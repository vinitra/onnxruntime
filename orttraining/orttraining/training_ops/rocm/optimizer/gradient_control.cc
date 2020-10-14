// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "core/providers/rocm/math/binary_elementwise_ops.h"
#include "core/providers/rocm/reduction/reduction_functions.h"
#include "core/providers/rocm/hip_allocator.h"
#include "common.h"
#include "gradient_control.h"

namespace onnxruntime {
namespace rocm {

#define REGISTER_IN_PLACE_TENSOR_ACCUMULATOR_TYPED(T, T_GRAD)                       \
  ONNX_OPERATOR_TYPED_KERNEL_EX(                                                    \
      InPlaceAccumulator,                                                           \
      kMSDomain,                                                                    \
      1,                                                                            \
      T##_##T_GRAD,                                                                 \
      kRocmExecutionProvider,                                                       \
      KernelDefBuilder()                                                            \
          .Alias(0, 0)                            /* Accumulate tensors in-place */ \
          .InputMemoryType<OrtMemTypeCPUInput>(2) /* Keep do_update in CPU */       \
          .TypeConstraint("T", DataTypeImpl::GetTensorType<T>())                    \
          .TypeConstraint("T_GRAD", DataTypeImpl::GetTensorType<T_GRAD>()),         \
      InPlaceAccumulator<T, T_GRAD>);

REGISTER_IN_PLACE_TENSOR_ACCUMULATOR_TYPED(float, float)
REGISTER_IN_PLACE_TENSOR_ACCUMULATOR_TYPED(float, MLFloat16)
REGISTER_IN_PLACE_TENSOR_ACCUMULATOR_TYPED(MLFloat16, MLFloat16)
REGISTER_IN_PLACE_TENSOR_ACCUMULATOR_TYPED(MLFloat16, float)

template <typename T>
Status ZeroGradient<T>::ComputeInternal(OpKernelContext* ctx) const {
  const Tensor& old_gradient = *ctx->Input<Tensor>(0);
  Tensor& zero_gradient = *ctx->Output(0, old_gradient.Shape());

  HIP_RETURN_IF_ERROR(hipMemsetAsync(
      zero_gradient.template MutableData<T>(),
      0,
      zero_gradient.Shape().Size() * sizeof(T)));

  return Status::OK();
}

#define REGISTER_ZERO_GRADIENT_TYPED(T)                           \
  ONNX_OPERATOR_TYPED_KERNEL_EX(                                  \
      ZeroGradient,                                               \
      kMSDomain,                                                  \
      1,                                                          \
      T,                                                          \
      kRocmExecutionProvider,                                     \
      KernelDefBuilder()                                          \
          .Alias(0, 0) /* Zero out gradients in-place */          \
          .TypeConstraint("T1", DataTypeImpl::GetTensorType<T>()) \
          .TypeConstraint("T2", DataTypeImpl::AllTensorTypes()),  \
      ZeroGradient<T>);
REGISTER_ZERO_GRADIENT_TYPED(float)
REGISTER_ZERO_GRADIENT_TYPED(MLFloat16)

template <typename T, typename T_GRAD>
Status InPlaceAccumulator<T, T_GRAD>::ComputeInternal(OpKernelContext* ctx) const {
  typedef typename ToHipType<T>::MappedType HipT;
  typedef typename ToHipType<T_GRAD>::MappedType HipT_GRAD;

  const Tensor& left_addee_buffer = *ctx->Input<Tensor>(0);
  const Tensor& right_addee_buffer = *ctx->Input<Tensor>(1);
  const Tensor* do_update_tensor = ctx->Input<Tensor>(2);
  Tensor& accumulation_output = *ctx->Output(0, left_addee_buffer.Shape());

  if (do_update_tensor) {
    const bool do_update = *(do_update_tensor->template Data<bool>());
    if (!do_update) {
      ORT_RETURN_IF_ERROR(CopyIfNotSameBuffer<T>(left_addee_buffer, accumulation_output));
      return Status::OK();
    }
  }
  InPlaceAccumulatorImpl(
      reinterpret_cast<const HipT*>(left_addee_buffer.template Data<T>()),
      reinterpret_cast<const HipT_GRAD*>(right_addee_buffer.template Data<T_GRAD>()),
      reinterpret_cast<HipT*>(accumulation_output.template MutableData<T>()),
      right_addee_buffer.Shape().Size());

  return Status::OK();
}

}  // namespace rocm
}  // namespace onnxruntime
