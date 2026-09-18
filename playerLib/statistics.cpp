// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   statistics.cpp
 *
 * @brief Definition of GITS file statistics.
 *
 */

#include "statistics.h"
#include "scheduler.h"
#include "gits.h"
#include "function.h"
#include "../OpenGL/common/include/openglArguments.h"

#include "yaml-cpp/yaml.h"
#include <filesystem>
#include <array>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

gits::CStatistics::CStatistics()
    : _init(false), _framesNum(0), _callsNum(0), _initCallsNum(0), _appCallsNum(0) {}

void gits::CStatistics::AddToken(const gits::CToken& token) {
  // check if API call
  if (token.Id() >= CToken::ID_OPENGL) {
    _callsNum++;
    if (_init) {
      _initCallsNum++;
    } else {
      _appCallsNum++;
    }

    const CFunction& function = static_cast<const CFunction&>(token);

    // create statistic tree date for that API call
    auto& callStats = _libraryStats[function.LibraryId()];
    TCall& call = callStats[function.Name()];

    // update API call data
    call.num++;
    call.currFrameNum++;
    if (call.currFrame != _framesNum + 1) {
      call.framesNum++;
      call.currFrame = _framesNum + 1;
    }

    // update IDs database
    _callsIds.insert(function.Id());
  }

  if (token.Id() == CToken::ID_INIT_START) {
    // library state initialization started
    _init = true;
  }

  if (token.Id() == CToken::ID_INIT_END) {
    // library state initialization ended
    _init = false;
  }

  if (token.Id() == CToken::ID_FRAME_END) {
    // end of the screen frame detected
    for (auto& lib : _libraryStats) {
      for (auto& call : lib.second) {
        call.second.NewFrame();
      }
    }
    _framesNum++;
  }
}

void gits::CStatistics::Get(CScheduler& scheduler, CStatsComputer& comp) {
  // add every API call from scheduler to statistics tree
  while (!scheduler.Run(comp))
    ;

  // reset init status
  _init = false;

  // update statistics database
  for (auto& lib : _libraryStats) {
    for (auto& call : lib.second) {
      if (call.second.currFrameNum > 0 && call.second.numPerFrameMin == 0xFFFFFFFF) {
        call.second.NewFrame();
      }
    }
  }

  // add skipped calls
  auto& inst = CGits::Instance();
  auto& skippedCalls = inst.FilePlayer().SkippedCalls();

  for (auto& skip : skippedCalls) {
    std::unique_ptr<CFunction> function(
        dynamic_cast<CFunction*>(inst.TokenCreate(CId(static_cast<uint16_t>(skip.first)))));

    if (function == nullptr) {
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }

    // create statistic tree data for that API call
    auto& callStats = _libraryStats[function->LibraryId()];
    TCall& call = callStats[function->Name()];

    // update API call data
    call.num = skip.second;
    call.skipped = true;

    _callsNum += call.num;
    _appCallsNum += call.num;
  }
}

void gits::CStatistics::Print() const {

  std::string filePath("statistics.yml");
  std::ofstream stream(filePath);
  GITS_ASSERT(stream.good(), "StatisticsService - failed to create file: " + filePath);

  YAML::Node output;
  YAML::Node stats = output["Statistics"];
  stats["ApiFunctionsNum"] = _callsIds.size();
  stats["FramesNum"] = _framesNum;
  stats["CallsNum"] = _callsNum;
  stats["StateInitCallsNum"] = _initCallsNum;
  stats["ApplicationCallsNum"] = _appCallsNum;
  stats["AvgAppCallsNumPerFrame"] = _appCallsNum / std::max(_framesNum, 1U);

  for (const auto& lib : _libraryStats) {
    const auto& library = CGits::Instance().Library(lib.first);
    YAML::Node apiCalls = output["ApiCalls"][library.Name()];
    for (const auto& call : lib.second) {
      YAML::Node callNode;
      callNode["Name"] = call.first;
      callNode["Num"] = call.second.num;
      callNode["FrNum"] = call.second.framesNum;
      callNode["MinPFr"] = call.second.numPerFrameMin;
      callNode["MaxPFr"] = call.second.numPerFrameMax;
      apiCalls.push_back(callNode);
    }
  }

  stream << output;
  LOG_INFO << "Statistics printed into statistics.yml file.";
}

gits::CStatistics::TCall::TCall()
    : num(0),
      numPerFrameMin(0xFFFFFFFF),
      numPerFrameMax(0),
      framesNum(0),
      currFrame(0),
      currFrameNum(0),
      skipped(false) {}

void gits::CStatistics::TCall::NewFrame() {
  numPerFrameMin = std::min(numPerFrameMin, currFrameNum);
  numPerFrameMax = std::max(numPerFrameMax, currFrameNum);
  currFrameNum = 0;
}

void gits::CStatsComputer::Run(CToken& token) {
  _stats.AddToken(token);
}
