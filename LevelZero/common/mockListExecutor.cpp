// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mockListExecutor.h"

namespace gits {
namespace l0 {
MockListExecutor::MockListExecutor(
    CStateDynamic& sd,
    CDriver& driver,
    const std::vector<ze_command_list_handle_t>& commandListsToSynchronize,
    const std::vector<ze_command_list_handle_t>& commandLists) {
  for (const auto& hCommandList : commandListsToSynchronize) {
    const auto& cmdListHandle = sd.Get<CCommandListState>(hCommandList, EXCEPTION_MESSAGE);
    actionsRequireToFinish.push_back(cmdListHandle.mockList);
  }
  for (const auto& hCommandList : commandLists) {
    const auto& cmdListHandle = sd.Get<CCommandListState>(hCommandList, EXCEPTION_MESSAGE);
    otherActions.push_back(cmdListHandle.mockList);
  }
  for (const auto& event : sd.Map<CEventState>()) {
    eventSignalMap[event.first] =
        driver.inject.zeEventQueryStatus(event.first) == ZE_RESULT_SUCCESS;
  }
}

bool MockListExecutor::CheckListsAreFinished(
    std::vector<std::pair<std::vector<CCommandListState::Action>::iterator,
                          std::vector<CCommandListState::Action>::iterator>>& iterators) const {
  const auto notFinished = std::any_of(iterators.begin(), iterators.end(),
                                       [](const auto& pair) { return pair.first != pair.second; });
  return !notFinished;
}

bool MockListExecutor::Run() {
  std::vector<std::pair<std::vector<CCommandListState::Action>::iterator,
                        std::vector<CCommandListState::Action>::iterator>>
      mainIterators;
  std::vector<std::pair<std::vector<CCommandListState::Action>::iterator,
                        std::vector<CCommandListState::Action>::iterator>>
      otherIterators;

  std::vector<std::vector<CCommandListState::Action>::iterator> previousState;
  std::vector<std::vector<CCommandListState::Action>::iterator> currentState;

  for (auto& main : actionsRequireToFinish) {
    mainIterators.push_back(std::make_pair(main.begin(), main.end()));
  }
  for (auto& main : otherActions) {
    otherIterators.push_back(std::make_pair(main.begin(), main.end()));
  }

  while (!CheckListsAreFinished(mainIterators)) {
    currentState.clear();
    for (auto& actions : mainIterators) {
      actions.first = Execute(actions.first, actions.second);
      currentState.push_back(actions.first);
    }
    for (auto& actions : otherIterators) {
      actions.first = Execute(actions.first, actions.second);
      currentState.push_back(actions.first);
    }
    if (currentState != previousState) {
      previousState = currentState;
    } else {
      return false;
    }
  }

  return true;
}

std::vector<CCommandListState::Action>::iterator MockListExecutor::Execute(
    std::vector<CCommandListState::Action>::iterator& actionIterator,
    std::vector<CCommandListState::Action>::iterator& endIterator) {
  while (actionIterator != endIterator) {
    switch (actionIterator->type) {
    case CCommandListState::Action::Type::Normal: {
      if (!actionIterator->waitEvents.empty()) {
        for (const auto& waitEvent : actionIterator->waitEvents) {
          if (!eventSignalMap[waitEvent]) {
            if (!SD().Exists<CEventState>(waitEvent)) {
              eventSignalMap[waitEvent] = true;
              continue;
            }
            return actionIterator;
          }
        }
      }
      if (actionIterator->signalEvent) {
        eventSignalMap[actionIterator->signalEvent] = true;
      }
      break;
    }
    case CCommandListState::Action::Type::Reset: {
      eventSignalMap[actionIterator->signalEvent] = false;
      break;
    }
    case CCommandListState::Action::Type::Signal: {
      eventSignalMap[actionIterator->signalEvent] = true;
      break;
    }
    }
    actionIterator++;
  }
  return actionIterator;
}

} // namespace l0
} // namespace gits
