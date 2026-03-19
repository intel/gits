// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "subcaptureRecorder.h"
#include "configurator.h"
#include "log.h"

#include <filesystem>
#include <string>

namespace gits {
namespace DirectX {

SubcaptureRecorder::SubcaptureRecorder() {
  const gits::Configuration& config = Configurator::Get();

  if (!config.directx.features.subcapture.enabled ||
      config.directx.features.subcapture.executionSerialization) {
    return;
  }

  Configurator::PrepareSubcapturePath();
  std::filesystem::path subcapturePath = config.common.player.subcapturePath;
  std::string commandListExecutions = config.directx.features.subcapture.commandListExecutions;
  if (!commandListExecutions.empty()) {
    std::string path = subcapturePath.parent_path().string();
    path += "/" + config.common.player.streamDir.filename().string();
    path += "_frames_" + config.directx.features.subcapture.frames;
    path += "_executions_" + commandListExecutions;
    const_cast<std::filesystem::path&>(config.common.player.subcapturePath) = path;
    subcapturePath = path;
    commandListSubcapture_ = true;
  }

  recorder_.reset(
      new stream::StreamWriter(subcapturePath, config.directx.features.subcapture.compressionType));

  copyAuxiliaryFiles();
}

SubcaptureRecorder::~SubcaptureRecorder() {
  finishRecording();
}

void SubcaptureRecorder::record(const stream::CommandSerializer& commandSerializer) {
  recorder_->Record(commandSerializer);
}

void SubcaptureRecorder::finishRecording() {
  if (!finished_) {
    recorder_->Close();
    LOG_INFO << "Subcapture recording finished";
    finished_ = true;
  }
}

void SubcaptureRecorder::copyAuxiliaryFiles() {
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
