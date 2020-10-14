// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "core/providers/rocm/nn/dropout.h"

namespace onnxruntime {
namespace rocm {

ONNX_OPERATOR_KERNEL_EX(
    Dropout,
    kOnnxDomain,
    12,
    kRocmExecutionProvider,
    KernelDefBuilder()
        .TypeConstraint("T", DataTypeImpl::AllIEEEFloatTensorTypes())
        .TypeConstraint("T1", DataTypeImpl::AllIEEEFloatTensorTypes())
        .TypeConstraint("T2", DataTypeImpl::GetTensorType<bool>())
        .InputMemoryType<OrtMemTypeCPUInput>(1)
        .InputMemoryType<OrtMemTypeCPUInput>(2),
    Dropout<false>);

}  // namespace rocm
}  // namespace onnxruntime
