// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"

#include "subcaptureRecorder.h"
#include "commandsAuto.h"
#include "raytracingBuildInputsHash.h"

#include <optional>

namespace gits {
namespace DirectX {

class StateTrackingService;

class AccelerationStructuresPrebuildInfoService {
public:
  AccelerationStructuresPrebuildInfoService(StateTrackingService& stateService,
                                            SubcaptureRecorder& recorder)
      : m_StateService(stateService), m_Recorder(recorder) {}

  void AddPrebuildInfo(ID3D12Device5GetRaytracingAccelerationStructurePrebuildInfoCommand& c);
  std::optional<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO> GetPrebuildInfoForInputs(
      const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs) const;
  void RestorePrebuildInfos();

private:
  struct PrebuildInfoEntry {
    CommandKey PrebuildCommandKey{};
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO Info{};
  };

  void StripRaytracingBuildInputs(
      PointerArgument<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS>& inputArgument);
  void RecordDummyResource();

private:
  StateTrackingService& m_StateService;
  SubcaptureRecorder& m_Recorder;
  RaytracingBuildInputsMap<PrebuildInfoEntry> m_PrebuildInfoEntries;
  ObjectKey m_DummyResourceKey{};
};

} // namespace DirectX
} // namespace gits
