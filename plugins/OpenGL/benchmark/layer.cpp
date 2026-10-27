// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "layer.h"

namespace gits::opengl {

namespace {

// Counts only swaps that actually ran and succeeded, and not the ones replayed as part of state
// restore.
template <typename SwapCommand>
bool IsCountedFrame(const SwapCommand& command) {
  return !command.m_Skip && command.m_Return.Value != 0 &&
         (command.m_Key & STATE_RESTORE_KEY_MASK) == 0;
}

// glXSwapBuffers returns nothing, so every one that ran ends a frame.
bool IsCountedFrame(const glXSwapBuffersCommand& command) {
  return !command.m_Skip && (command.m_Key & STATE_RESTORE_KEY_MASK) == 0;
}

} // namespace

BenchmarkLayer::BenchmarkLayer(const BenchmarkConfig& cfg, gits::MessageBus& msgBus)
    : Layer("BenchmarkPlugin"), m_CpuFrameBenchmarkService(cfg, msgBus) {}

void BenchmarkLayer::Pre(wglCreateContextCommand&) {
  m_CpuFrameBenchmarkService.OnStart();
}

void BenchmarkLayer::Pre(wglCreateContextAttribsARBCommand&) {
  m_CpuFrameBenchmarkService.OnStart();
}

void BenchmarkLayer::Post(wglSwapBuffersCommand& command) {
  if (IsCountedFrame(command)) {
    m_CpuFrameBenchmarkService.OnFrameEnd();
  }
}

void BenchmarkLayer::Post(wglSwapLayerBuffersCommand& command) {
  if (IsCountedFrame(command)) {
    m_CpuFrameBenchmarkService.OnFrameEnd();
  }
}

void BenchmarkLayer::Pre(glXCreateContextCommand&) {
  m_CpuFrameBenchmarkService.OnStart();
}

void BenchmarkLayer::Pre(glXCreateNewContextCommand&) {
  m_CpuFrameBenchmarkService.OnStart();
}

void BenchmarkLayer::Pre(glXCreateContextAttribsARBCommand&) {
  m_CpuFrameBenchmarkService.OnStart();
}

void BenchmarkLayer::Post(glXSwapBuffersCommand& command) {
  if (IsCountedFrame(command)) {
    m_CpuFrameBenchmarkService.OnFrameEnd();
  }
}

} // namespace gits::opengl
