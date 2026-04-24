// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

namespace gits {
namespace DirectX {

class SubcaptureRange {
public:
  SubcaptureRange();

  void FrameEnd(bool stateRestore);
  bool IsFrameRangeStart(bool stateRestore);
  void ExecutionStart();
  void ExecutionEnd();
  bool IsExecutionRangeStart();
  bool InRange();
  bool CommandListSubcapture();

private:
  unsigned m_ExecutionCount{};
  unsigned m_ExecutionRangeStart{};
  unsigned m_ExecutionRangeEnd{};
  bool m_InsideExecution{};
  bool m_ZeroOrFirstFrame{true};

  bool m_InFrameRange{};
  bool m_TrimmingMode{};
  unsigned m_StartFrame{};
  unsigned m_EndFrame{};
  unsigned m_CurrentFrame{1};
};

} // namespace DirectX
} // namespace gits
