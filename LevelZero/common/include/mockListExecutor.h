// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "l0Header.h"
#include "l0Log.h"
#include "l0StateDynamic.h"
#include "l0Drivers.h"
#include <unordered_map>
#include <utility>

namespace gits {
namespace l0 {
class MockListExecutor {
private:
  std::vector<std::vector<CCommandListState::Action>> actionsRequireToFinish;
  std::vector<std::vector<CCommandListState::Action>> otherActions;
  std::unordered_map<ze_event_handle_t, bool> eventSignalMap;
  bool CheckListsAreFinished(
      std::vector<std::pair<std::vector<CCommandListState::Action>::iterator,
                            std::vector<CCommandListState::Action>::iterator>>& iterators) const;
  std::vector<CCommandListState::Action>::iterator Execute(
      std::vector<CCommandListState::Action>::iterator& actionIterator,
      std::vector<CCommandListState::Action>::iterator& endIterator);

public:
  MockListExecutor(CStateDynamic& sd,
                   CDriver& driver,
                   const std::vector<ze_command_list_handle_t>& commandListsToSynchronize,
                   const std::vector<ze_command_list_handle_t>& commandLists);
  bool Run();
};

} // namespace l0
} // namespace gits
