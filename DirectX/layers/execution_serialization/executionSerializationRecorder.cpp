// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "executionSerializationRecorder.h"
#include "directXApiIfaceExecutionSerialization.h"

#include <filesystem>

namespace gits {
namespace DirectX {

ExecutionSerializationRecorder::ExecutionSerializationRecorder() {

  const gits::Configuration& config = Configurator::Get();

  if (!config.directx.features.subcapture.enabled ||
      !config.directx.features.subcapture.executionSerialization) {
    return;
  }

  std::string subcapturePath = config.common.player.subcapturePath.string();
  subcapturePath += "_serialized";
  const_cast<std::filesystem::path&>(config.common.player.subcapturePath) = subcapturePath;

  CGits::Instance().apis.UseApi3dIface(
      std::shared_ptr<ApisIface::Api3d>(new DirectXApiIfaceExecutionSerialization()));
  CRecorder::Instance();

  copyAuxiliaryFiles();
}

void ExecutionSerializationRecorder::record(CToken* token) {
  CRecorder::Instance().Schedule(token);
}

void ExecutionSerializationRecorder::frameEnd() {
  CRecorder::Instance().FrameEnd();
}

bool ExecutionSerializationRecorder::isRunning() {
  return CRecorder::Instance().Running();
}

void ExecutionSerializationRecorder::copyAuxiliaryFiles() {
  std::filesystem::path streamDir = Configurator::Get().common.player.streamDir;
  std::filesystem::path subcapturePath = Configurator::Get().common.player.subcapturePath;
  if (std::filesystem::exists(streamDir / "raytracingArraysOfPointers.dat")) {
    std::filesystem::copy(streamDir / "raytracingArraysOfPointers.dat", subcapturePath,
                          std::filesystem::copy_options::overwrite_existing);
  }
  if (std::filesystem::exists(streamDir / "executeIndirectRaytracing.txt")) {
    std::filesystem::copy(streamDir / "executeIndirectRaytracing.txt", subcapturePath,
                          std::filesystem::copy_options::overwrite_existing);
  }
  if (std::filesystem::exists(streamDir / "resourcePlacementData.dat")) {
    std::filesystem::copy(streamDir / "resourcePlacementData.dat", subcapturePath,
                          std::filesystem::copy_options::overwrite_existing);
  }
  if (std::filesystem::exists(streamDir / "addressRanges.txt")) {
    std::filesystem::copy(streamDir / "addressRanges.txt", subcapturePath,
                          std::filesystem::copy_options::overwrite_existing);
  }
  if (std::filesystem::exists(streamDir / "DirectStorageResources.bin")) {
    std::filesystem::copy(streamDir / "DirectStorageResources.bin", subcapturePath,
                          std::filesystem::copy_options::overwrite_existing);
  }
}

} // namespace DirectX
} // namespace gits
