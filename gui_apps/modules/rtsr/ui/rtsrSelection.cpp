// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "rtsr/ui/rtsrSelection.h"

#include <algorithm>

namespace gits::gui {

void PlotSelection::Clamp() {
  if (m_PointCount == 0) {
    m_Index = 0;
    m_EffectiveWindowLength = 1;
    return;
  }
  m_Index = std::min(m_Index, m_PointCount - 1);
  m_WindowLength = std::max<size_t>(m_WindowLength, 1);
  m_EffectiveWindowLength = std::min(m_WindowLength, m_PointCount - m_Index);
}

void PlotSelection::SetPointCount(size_t count) {
  m_PointCount = count;
  Clamp();
}

void PlotSelection::SetIndex(size_t index) {
  m_Index = index;
  Clamp();
}

bool PlotSelection::TrySetIndex(size_t index) {
  if (m_PointCount == 0 || index >= m_PointCount) {
    return false;
  }
  m_Index = index;
  Clamp();
  return true;
}

void PlotSelection::SetWindowLength(size_t length) {
  m_WindowLength = std::max<size_t>(length, 1);
  Clamp();
}

size_t PlotSelection::MaxWindowLength() const {
  if (m_PointCount == 0 || m_Index >= m_PointCount) {
    return 1;
  }
  return m_PointCount - m_Index;
}

size_t PlotSelection::End() const {
  // Inclusive, hence the -1: a window of length one covers the index alone.
  return m_Index + m_EffectiveWindowLength - 1;
}

void PlotSelection::Reset() {
  m_Selected = false;
  m_PointCount = 0;
  m_Index = 0;
  m_WindowLength = 1;
  m_EffectiveWindowLength = 1;
}

bool operator==(const PlotSelection& lhs, const PlotSelection& rhs) {
  return lhs.m_Selected == rhs.m_Selected && lhs.m_PointCount == rhs.m_PointCount &&
         lhs.m_Index == rhs.m_Index && lhs.m_WindowLength == rhs.m_WindowLength &&
         lhs.m_EffectiveWindowLength == rhs.m_EffectiveWindowLength;
}

bool operator!=(const PlotSelection& lhs, const PlotSelection& rhs) {
  return !(lhs == rhs);
}

} // namespace gits::gui
