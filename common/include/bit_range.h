// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <string>
#include <vector>
#include <istream>

class BitRange {
public:
  explicit BitRange(bool full = false);
  explicit BitRange(const std::string& range_spec, size_t block_size = 100000);
  bool operator[](size_t index) const;
  bool empty() const {
    return empty_;
  }
  bool full() const {
    return full_;
  }
  std::string StrValue() const {
    return range_spec_;
  };

private:
  void recompute_block() const;
  bool full_;
  bool empty_;
  mutable size_t base_;
  mutable std::vector<bool> bits_;
  std::string range_spec_;
};

std::istream& operator>>(std::istream& lhs, BitRange& rhs);
