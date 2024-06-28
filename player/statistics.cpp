// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
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

    if (Config::Get().common.player.statsVerb) {
      unsigned argc = function.ArgumentCount();
      for (unsigned i = 0; i < argc; ++i) {
        if (const OpenGL::CGLenum* ptr =
                dynamic_cast<const OpenGL::CGLenum*>(&function.Argument(i))) {
          _glenumsUsed.insert(ptr->Value());
        }
      }
    }
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

void gits::CStatistics::Print(bool verbose) const {
  using namespace std;

  cout << "Statistics" << endl;
  cout << "==========" << endl;
  cout << "API functions Num             " << _callsIds.size() << endl;
  if (_framesNum) {
    cout << "Frames Num                    " << _framesNum << endl;
  }
  cout << "Calls Num                     " << _callsNum << endl;
  cout << "State Init Calls Num          " << _initCallsNum << endl;
  cout << "Application Calls Num         " << _appCallsNum << endl;
  cout << "Avg. App. Calls Num per Frame ";
  if (_framesNum == 0) {
    cout << "NaN" << endl;
  } else {
    cout << _appCallsNum / _framesNum << endl;
  }
  cout << endl;

  std::array<string, 5> columnHeaders = {{"Name", "Num", "FrNum", "MINpFr", "MAXpFr"}};
  CAsciiTable<5> table(columnHeaders);
  for (const auto& lib : _libraryStats) {
    const auto& library = CGits::Instance().Library(lib.first);
    table.AddOneCellRow(library.Name()); // Library name is e.g. "Vulkan" or "OpenCL".

    for (const auto& call : lib.second) {
      std::array<string, 5> row;
      row[0] = "  " + call.first;
      row[1] = std::to_string(call.second.num);

      if (call.second.skipped) {
        row[2] = "???";
        row[3] = "???";
        row[4] = "???";
      } else {
        row[2] = std::to_string(call.second.framesNum);
        row[3] = std::to_string(call.second.numPerFrameMin);
        row[4] = std::to_string(call.second.numPerFrameMax);
      }

      table.AddRow(row);
    }
  }
  table.Print(cout);

  if (verbose) {
    if (!_glenumsUsed.empty()) {
      std::cout << "\n\nFollowing GLenum values are used in the stream:\n";
    }
    std::set<unsigned>::const_iterator iter = _glenumsUsed.begin();
    std::set<unsigned>::const_iterator end = _glenumsUsed.end();
    while (iter != end) {
      for (int i = 0; i < 10 && iter != end; ++i, ++iter) {
        std::cout << std::hex << std::showbase << std::setfill('0') << std::setw(6) << std::internal
                  << *iter << std::dec << "  ";
      }
      std::cout << '\n';
    }
  }
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
