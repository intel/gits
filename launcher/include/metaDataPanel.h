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

#include <string>
#include <filesystem>
#include <mutex>
#include <atomic>

namespace gits::gui {
class MetaDataPanel : public BasePanel {
public:
  MetaDataPanel(ISharedContext& sharedContext);
  void InvalidateMetaData();
  void Render() override;

private:
  void LoadMetaData(std::filesystem::path streamPath);
  void ClearStats();
  void AppendStats(std::string str);
  void FillStatsEditor();
  bool MetaDataNeedsUpdating = true;
  STREAM_META_DATA MetaData;
  std::string Stats;
  std::mutex StatsMutex;
  std::atomic<bool> StatsGatheringInProgress = false;
};
} // namespace gits::gui
