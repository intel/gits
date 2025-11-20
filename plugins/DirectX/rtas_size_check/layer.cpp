// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "layer.h"
#include "pluginUtils.h"
#include <wrl/client.h>

namespace gits {
namespace DirectX {

RtasSizeCheckLayer::RtasSizeCheckLayer(MessageBus* msgBus)
    : Layer("RtasSizeCheck"), msgBus_(msgBus), lastCaptureTimePrebuildInfo_{} {}

void RtasSizeCheckLayer::pre(
    ID3D12Device5GetRaytracingAccelerationStructurePrebuildInfoCommand& command) {
  lastCaptureTimePrebuildInfo_ = *command.pInfo_.value;
}

void RtasSizeCheckLayer::post(
    ID3D12Device5GetRaytracingAccelerationStructurePrebuildInfoCommand& command) {
  ID3D12Device5* device = command.object_.value;

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO playbackPrebuildInfo{};
  if (command.pInfo_.value) {
    playbackPrebuildInfo = *command.pInfo_.value;
  }

  // If it was skipped then call it here
  if (command.skip) {
    device->GetRaytracingAccelerationStructurePrebuildInfo(command.pDesc_.value,
                                                           &playbackPrebuildInfo);
  }

  if (playbackPrebuildInfo.ResultDataMaxSizeInBytes >
      lastCaptureTimePrebuildInfo_.ResultDataMaxSizeInBytes) {
    logT(msgBus_,
         "ERROR: RTAS ResultDataMaxSizeInBytes during playback is bigger than during capture on "
         "command ",
         command.key);
    logT(msgBus_,
         "Playback ResultDataMaxSizeInBytes: ", playbackPrebuildInfo.ResultDataMaxSizeInBytes);
    logT(msgBus_, "Capture ResultDataMaxSizeInBytes: ",
         lastCaptureTimePrebuildInfo_.ResultDataMaxSizeInBytes);
  }

  if (playbackPrebuildInfo.ScratchDataSizeInBytes >
      lastCaptureTimePrebuildInfo_.ScratchDataSizeInBytes) {
    logT(msgBus_,
         "ERROR: RTAS ScratchDataSizeInBytes during playback is bigger than during capture on "
         "command ",
         command.key);
    logT(msgBus_, "Playback ScratchDataSizeInBytes: ", playbackPrebuildInfo.ScratchDataSizeInBytes);
    logT(msgBus_,
         "Capture ScratchDataSizeInBytes: ", lastCaptureTimePrebuildInfo_.ScratchDataSizeInBytes);
  }

  if (playbackPrebuildInfo.UpdateScratchDataSizeInBytes >
      lastCaptureTimePrebuildInfo_.UpdateScratchDataSizeInBytes) {
    logT(
        msgBus_,
        "ERROR: RTAS UpdateScratchDataSizeInBytes during playback is bigger than during capture on "
        "command ",
        command.key);
    logT(msgBus_, "Playback UpdateScratchDataSizeInBytes: ",
         playbackPrebuildInfo.UpdateScratchDataSizeInBytes);
    logT(msgBus_, "Capture UpdateScratchDataSizeInBytes: ",
         lastCaptureTimePrebuildInfo_.UpdateScratchDataSizeInBytes);
  }
  lastCaptureTimePrebuildInfo_ = {};
}

} // namespace DirectX
} // namespace gits
