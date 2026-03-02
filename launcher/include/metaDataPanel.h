// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "basePanel.h"
#include "metaDataActions.h"
#include "eventBus.h"

#include <string>
#include <filesystem>
#include <mutex>
#include <atomic>

namespace gits::gui {
class MetaDataPanel : public BasePanel {
public:
  MetaDataPanel();
  void Render() override;

private:
  void InvalidateMetaData();
  void LoadMetaData(std::filesystem::path streamPath);
  void ClearStats();
  void AppendStats(std::string str);
  void FillStatsEditor();

  // Event callbacks
  void StreamPathCallback(const Event& e);

  bool MetaDataNeedsUpdating = true;
  STREAM_META_DATA MetaData;
  std::string Stats;
  std::mutex StatsMutex;
  std::atomic<bool> StatsGatheringInProgress = false;
};
} // namespace gits::gui
