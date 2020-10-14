// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

namespace onnxruntime {

/**
  * Configuration information for a hip provider.
  *
  * Note: This struct is currently for internal use for Python API,
  *       not for C/C++/C#...APIs. 
  */
struct RocmProviderOptions {

  // use hip device with id=0 as default device.
  OrtDevice::DeviceId device_id = 0;

  // set default hip memory limitation to maximum finite value of size_t.
  size_t hip_mem_limit = std::numeric_limits<size_t>::max();

  // set default area extend strategy to KNextPowerOfTwo.
  onnxruntime::ArenaExtendStrategy arena_extend_strategy = onnxruntime::ArenaExtendStrategy::kNextPowerOfTwo;
};
}  // namespace onnxruntime
