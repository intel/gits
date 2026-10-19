// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <cstddef>

namespace gits::gui {

// What the user has picked in a plot: one point, or a window of consecutive
// points starting there.
//
// Positions are plain indices into the plot's data array. RTSR's original
// carried a parallel "value" space with a range offset so that a selection could
// follow a frame number as data scrolled, but that only works while the x values
// are gap free - which stops being true the moment measurement is paused. Here
// the axis is dense and following a particular frame is the panel's job, which
// it does by asking PerformanceStats for the index of a frame number.
class PlotSelection {
public:
  // Number of points currently in the data. The index and the window are
  // clamped to it, so shrinking data cannot leave a selection dangling.
  void SetPointCount(size_t count);

  void SetIndex(size_t index);
  // Leaves the selection untouched and returns false when the index is out of
  // range, which is what the previous and next buttons rely on at the ends.
  bool TrySetIndex(size_t index);
  size_t Index() const {
    return m_Index;
  }

  // Requested length, which is what the slider shows. The length actually in
  // effect is clamped against the end of the data and shows up through End().
  void SetWindowLength(size_t length);
  size_t WindowLength() const {
    return m_WindowLength;
  }
  size_t MaxWindowLength() const;

  // Inclusive index range of the selection. Meaningless unless IsSelected().
  size_t Begin() const {
    return m_Index;
  }
  size_t End() const;

  void SetSelected(bool selected) {
    m_Selected = selected;
  }
  bool IsSelected() const {
    return m_Selected;
  }

  void Reset();

  friend bool operator==(const PlotSelection& lhs, const PlotSelection& rhs);
  friend bool operator!=(const PlotSelection& lhs, const PlotSelection& rhs);

private:
  void Clamp();

  bool m_Selected{false};
  size_t m_PointCount{0};
  size_t m_Index{0};
  size_t m_WindowLength{1};
  size_t m_EffectiveWindowLength{1};
};

} // namespace gits::gui
