// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "unsupportedCommandRunner.h"

#include "log.h"

#include <cstdint>
#include <cstring>
#include <unordered_set>

namespace gits::opengl2 {

void UnsupportedCommandRunner::DecodeCommand() {
  // Mirrors UnsupportedCommandSerializer's flat [nameLength][name bytes] payload exactly - see
  // unsupportedCommandTypes.h. No offset tracking is needed: this is the entire payload, not
  // one field among several.
  uint32_t nameLength{};
  std::memcpy(&nameLength, m_Data, sizeof(nameLength));
  m_Name.assign(m_Data + sizeof(nameLength), nameLength);
}

void UnsupportedCommandRunner::Run() {
  // StreamReader::Run() calls every block's runners sequentially from one thread (see
  // streamReader.cpp) - unlike the recorder's ReportUnsupportedCommand, which must tolerate
  // concurrent GL calls from multiple application threads and so uses a
  // tbb::concurrent_unordered_set (recorder/unsupportedCommand.cpp), the player never calls
  // Run() concurrently with itself, so a plain, unsynchronized std::unordered_set is enough.
  static std::unordered_set<std::string> alreadyWarnedNames;
  if (alreadyWarnedNames.insert(m_Name).second) {
    LOG_WARNING << "Replay skipped unsupported OpenGL command (degraded fidelity): " << m_Name;
  }
}

} // namespace gits::opengl2
