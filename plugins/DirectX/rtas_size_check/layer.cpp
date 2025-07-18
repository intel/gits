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

RtasSizeCheckLayer::RtasSizeCheckLayer(CGits& gits) : Layer("RtasSizeCheck"), gits_(gits) {}

void RtasSizeCheckLayer::post(
    ID3D12Device5GetRaytracingAccelerationStructurePrebuildInfoCommand& command) {
  ID3D12Device5* device = command.object_.value;
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO playbackPrebuildInfo{};
  device->GetRaytracingAccelerationStructurePrebuildInfo(command.pDesc_.value,
                                                         &playbackPrebuildInfo);

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO* capturePrebuildInfo = command.pInfo_.value;

  if (playbackPrebuildInfo.ResultDataMaxSizeInBytes >
      capturePrebuildInfo->ResultDataMaxSizeInBytes) {
    logT(gits_,
         "ERROR: RTAS ResultDataMaxSizeInBytes during playback is bigger than during capture on "
         "command ",
         command.key);
    logT(gits_,
         "Playback ResultDataMaxSizeInBytes: ", playbackPrebuildInfo.ResultDataMaxSizeInBytes);
    logT(gits_,
         "Capture ResultDataMaxSizeInBytes: ", capturePrebuildInfo->ResultDataMaxSizeInBytes);
  }

  if (playbackPrebuildInfo.ScratchDataSizeInBytes > capturePrebuildInfo->ScratchDataSizeInBytes) {
    logT(gits_,
         "ERROR: RTAS ScratchDataSizeInBytes during playback is bigger than during capture on "
         "command ",
         command.key);
    logT(gits_, "Playback ScratchDataSizeInBytes: ", playbackPrebuildInfo.ScratchDataSizeInBytes);
    logT(gits_, "Capture ScratchDataSizeInBytes: ", capturePrebuildInfo->ScratchDataSizeInBytes);
  }

  if (playbackPrebuildInfo.UpdateScratchDataSizeInBytes >
      capturePrebuildInfo->UpdateScratchDataSizeInBytes) {
    logT(
        gits_,
        "ERROR: RTAS UpdateScratchDataSizeInBytes during playback is bigger than during capture on "
        "command ",
        command.key);
    logT(gits_, "Playback UpdateScratchDataSizeInBytes: ",
         playbackPrebuildInfo.UpdateScratchDataSizeInBytes);
    logT(gits_, "Capture UpdateScratchDataSizeInBytes: ",
         capturePrebuildInfo->UpdateScratchDataSizeInBytes);
  }
}

} // namespace DirectX
} // namespace gits
