// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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
    : Layer("RtasSizeCheck"), m_MsgBus(msgBus), m_LastCaptureTimePrebuildInfo{} {}

void RtasSizeCheckLayer::Pre(
    ID3D12Device5GetRaytracingAccelerationStructurePrebuildInfoCommand& command) {
  m_LastCaptureTimePrebuildInfo = *command.m_pInfo.Value;
}

void RtasSizeCheckLayer::Post(
    ID3D12Device5GetRaytracingAccelerationStructurePrebuildInfoCommand& command) {
  ID3D12Device5* device = command.m_Object.Value;

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO playbackPrebuildInfo{};
  if (command.m_pInfo.Value) {
    playbackPrebuildInfo = *command.m_pInfo.Value;
  }

  // If it was skipped then call it here
  if (command.Skip) {
    device->GetRaytracingAccelerationStructurePrebuildInfo(command.m_pDesc.Value,
                                                           &playbackPrebuildInfo);
  }

  if (playbackPrebuildInfo.ResultDataMaxSizeInBytes >
      m_LastCaptureTimePrebuildInfo.ResultDataMaxSizeInBytes) {
    logT(m_MsgBus,
         "ERROR: RTAS ResultDataMaxSizeInBytes during playback is bigger than during capture on "
         "command ",
         command.Key);
    logT(m_MsgBus,
         "Playback ResultDataMaxSizeInBytes: ", playbackPrebuildInfo.ResultDataMaxSizeInBytes);
    logT(m_MsgBus, "Capture ResultDataMaxSizeInBytes: ",
         m_LastCaptureTimePrebuildInfo.ResultDataMaxSizeInBytes);
  }

  if (playbackPrebuildInfo.ScratchDataSizeInBytes >
      m_LastCaptureTimePrebuildInfo.ScratchDataSizeInBytes) {
    logT(m_MsgBus,
         "ERROR: RTAS ScratchDataSizeInBytes during playback is bigger than during capture on "
         "command ",
         command.Key);
    logT(m_MsgBus,
         "Playback ScratchDataSizeInBytes: ", playbackPrebuildInfo.ScratchDataSizeInBytes);
    logT(m_MsgBus,
         "Capture ScratchDataSizeInBytes: ", m_LastCaptureTimePrebuildInfo.ScratchDataSizeInBytes);
  }

  if (playbackPrebuildInfo.UpdateScratchDataSizeInBytes >
      m_LastCaptureTimePrebuildInfo.UpdateScratchDataSizeInBytes) {
    logT(
        m_MsgBus,
        "ERROR: RTAS UpdateScratchDataSizeInBytes during playback is bigger than during capture on "
        "command ",
        command.Key);
    logT(m_MsgBus, "Playback UpdateScratchDataSizeInBytes: ",
         playbackPrebuildInfo.UpdateScratchDataSizeInBytes);
    logT(m_MsgBus, "Capture UpdateScratchDataSizeInBytes: ",
         m_LastCaptureTimePrebuildInfo.UpdateScratchDataSizeInBytes);
  }
  m_LastCaptureTimePrebuildInfo = {};
}

} // namespace DirectX
} // namespace gits
