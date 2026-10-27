// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandPrinter.h"

#include "configurator.h"

#include <chrono>
#include <cstdio>
#include <ctime>

namespace gits::opengl {

CommandPrinter::CommandPrinter(FastOStream& stream,
                               std::mutex& mutex,
                               const Command& command,
                               const char* name)
    : m_Stream(stream), m_Lock(mutex) {
  if (Configurator::Get().common.shared.trace.print.timestamp) {
    const auto now = std::chrono::system_clock::now();
    const auto seconds = std::chrono::system_clock::to_time_t(now);
    const auto micros =
        std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000;
    std::tm calendar{};
#ifdef _WIN32
    localtime_s(&calendar, &seconds);
#else
    localtime_r(&seconds, &calendar);
#endif
    char text[32];
    const size_t offset = std::strftime(text, sizeof(text), "%Y-%m-%d %H:%M:%S", &calendar);
    std::snprintf(text + offset, sizeof(text) - offset, ".%06d ", static_cast<int>(micros.count()));
    m_Stream << text;
  }
  m_Stream << command.m_Key << " T" << command.m_ThreadId << ' ' << name << '(';
}

void CommandPrinter::Finish(bool flush, const char* suffix) {
  if (!m_HasResult) {
    m_Stream << ')';
  }
  m_Stream << suffix << '\n';
  if (flush) {
    m_Stream.Flush();
  }
}

} // namespace gits::opengl
