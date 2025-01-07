// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   runner.cpp
* 
* @brief  GITS tokens runner.
* 
*/

#include "runner.h"
#include "function.h"
#include "argument.h"
#include "gits.h"
#include "config.h"
#include "log.h"

void gits::CRunner::Register(std::shared_ptr<CHandler> plugin) {
  _handlerList.push_back(plugin);
  _hasHandlers = true;
}

gits::CRunner::CRunner() {
  _hasHandlers = false;
}

static NOINLINE void log_played_token(gits::CToken& token) {
  using namespace gits;
  if (CFunction* func = dynamic_cast<CFunction*>(&token)) {
    Log(INFO) << "Processing API call: " << func->Name()
              << " [TID = " << CGits::Instance().CurrentThreadId() << "]";
  } else {
    Log(INFO) << "Processing Non-function token (" << TryToDemangle(typeid(token).name()).name
              << ") [TID = " << CGits::Instance().CurrentThreadId() << "]";
  }
}

gits::CRunner::TResultType gits::CRunner::operator()(CAction& action, CToken& token) const {
  if (Config::Get().common.player.logFncs) {
    log_played_token(token);
  }

  if (!_hasHandlers) {
    action.Run(token);
    return TResultType();
  }

  return RunWithHandlers(action, token);
}

gits::CRunner::TResultType gits::CRunner::RunWithHandlers(CAction& action, CToken& token) const {
  CFunction* func = dynamic_cast<CFunction*>(&token);
  if (func == nullptr) {
    action.Run(token);
    return TResultType();
  } else {
    // find handlers matching that token
    std::vector<CHandler*> matchingList;
    for (auto& handler : _handlerList) {
      if (handler->Match(*func)) {
        matchingList.push_back(handler.get());
      }
    }

    // run pre-handlers and determine if a token should be skipped
    auto skip = matchingList.size() ? CRunner::CHandler::SKIP_YES : CRunner::CHandler::SKIP_NO;
    auto schedule = true;
    for (auto& matching : matchingList) {
      auto pluginResults = matching->PreHandler(*func);
      skip = std::max(skip, pluginResults.skip);
      schedule = std::min(schedule, pluginResults.schedule);
    }

    // run token if not skipped
    if (skip == CRunner::CHandler::SKIP_NO) {
      action.Run(token);
    }

    // run pos-handlers
    for (auto& matching : matchingList) {
      matching->PostHandler(*func, skip != CRunner::CHandler::SKIP_NO);
    }

    return TResultType(skip, schedule);
  }
}

void gits::CAction::Run(CToken& token) {
  if (!Config::Get().common.player.nullRun) {
    token.Run();
  }
}
