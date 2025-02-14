// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "analyzerLayerAuto.h"
#include "config.h"
#include "gits.h"

#include <fstream>
#include <sstream>

namespace gits {
namespace DirectX {

AnalyzerLayer::AnalyzerLayer() : Layer("Analyzer") {

  const std::string& frames = Config::Get().directx.features.subcapture.frames;
  size_t pos = frames.find("-");
  try {
    if (pos != std::string::npos) {
      startFrame_ = std::stoi(frames.substr(0, pos));
      endFrame_ = std::stoi(frames.substr(pos + 1));
    } else {
      startFrame_ = std::stoi(frames);
      endFrame_ = startFrame_;
    }
  } catch (...) {
    throw Exception("Invalid subcapture range: '" +
                    Config::Get().directx.features.subcapture.frames + "'");
  }
};

void AnalyzerLayer::post(IDXGISwapChainPresentCommand& c) {
  if (c.Flags_.value & DXGI_PRESENT_TEST || c.key & Command::stateRestoreKeyMask) {
    return;
  }
  unsigned currentFrame = CGits::Instance().CurrentFrame();
  if (currentFrame == startFrame_ - 1) {
    Log(INFOV) << "Start analysis frame " << currentFrame + 1 << " call " << c.key;
    inRange_ = true;
  } else if (currentFrame == endFrame_) {
    if (inRange_) {
      inRange_ = false;

      std::ofstream out(getAnalysisFileName());
      for (unsigned key : commandListsForRestore_) {
        out << key << "\n";
      }
    }
  }
}

std::string AnalyzerLayer::getAnalysisFileName() {
  const Config& config = Config::Get();
  std::stringstream fileName;
  fileName << config.common.player.streamDir.filename().string() << "_frames-"
           << config.directx.features.subcapture.frames;
  fileName << "_analysis.txt";

  return fileName.str();
}

void AnalyzerLayer::post(IDXGISwapChain1Present1Command& c) {
  if (c.PresentFlags_.value & DXGI_PRESENT_TEST || c.key & Command::stateRestoreKeyMask) {
    return;
  }
  unsigned currentFrame = CGits::Instance().CurrentFrame();
  if (currentFrame == startFrame_ - 1) {
    Log(INFOV) << "Start analysis frame " << currentFrame + 1 << " call " << c.key;
    inRange_ = true;
  } else if (CGits::Instance().CurrentFrame() == endFrame_) {
    if (inRange_) {
      inRange_ = false;

      std::ofstream out(getAnalysisFileName());
      for (unsigned key : commandListsForRestore_) {
        out << key << "\n";
      }
    }
  }
}

void AnalyzerLayer::post(ID3D12CommandQueueExecuteCommandListsCommand& c) {
  if (inRange_) {
    for (unsigned i = 0; i < c.NumCommandLists_.value; ++i) {
      unsigned commandListKey = c.ppCommandLists_.keys[i];
      auto it = commandListsResetBeforeExecution_.find(commandListKey);
      if (it == commandListsResetBeforeExecution_.end()) {
        commandListsForRestore_.insert(commandListKey);
      }
      commandListsExecuted_.insert(commandListKey);
    }
  }
}

void AnalyzerLayer::post(ID3D12GraphicsCommandListResetCommand& c) {
  if (inRange_) {
    unsigned commandListKey = c.object_.key;
    auto it = commandListsExecuted_.find(commandListKey);
    if (it == commandListsExecuted_.end()) {
      commandListsResetBeforeExecution_.insert(commandListKey);
    }
    commandListsReset_.insert(commandListKey);
  }
}

} // namespace DirectX
} // namespace gits
