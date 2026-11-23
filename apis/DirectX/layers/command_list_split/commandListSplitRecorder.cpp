// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandListSplitRecorder.h"
#include "configurator.h"
#include "log.h"
#include "exception.h"

#include <filesystem>

namespace gits {
namespace DirectX {

CommandListSplitRecorder::CommandListSplitRecorder() {
  gits::Configuration& config = Configurator::GetMutable();

  if (!config.common.player.subcapture.enabled ||
      config.common.player.subcapture.directx.commandListSplit.empty()) {
    return;
  }

  config.common.player.subcapture.frames = "";
  Configurator::PrepareSubcapturePath();
  std::string subcapturePath = config.common.player.subcapturePath.string();
  const std::string sub = "frames-";
  const auto pos = subcapturePath.rfind(sub);
  if (pos != std::string::npos) {
    subcapturePath.erase(subcapturePath.begin() + pos, subcapturePath.end());
  }
  subcapturePath += "split";
  config.common.player.subcapturePath = subcapturePath;

  m_Recorder.reset(
      new stream::StreamWriter(subcapturePath, config.common.player.subcapture.compressionType));

  CopyAuxiliaryFiles();
}

CommandListSplitRecorder::~CommandListSplitRecorder() {
  try {
    FinishRecording();
  } catch (...) {
    topmost_exception_handler("CommandListSplitRecorder::~CommandListSplitRecorder");
  }
}

void CommandListSplitRecorder::Record(const stream::CommandSerializer& commandSerializer) {
  if (m_Recorder) {
    m_Recorder->Record(commandSerializer);
  }
}

void CommandListSplitRecorder::FinishRecording() {
  if (!m_Finished && m_Recorder) {
    m_Recorder->Close();
    LOG_INFO << "Command list split recording finished";
    m_Finished = true;
  }
}

void CommandListSplitRecorder::CopyAuxiliaryFiles() {
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
