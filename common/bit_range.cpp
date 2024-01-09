// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "bit_range.h"
#include <sstream>
#include <algorithm>
#include <stdexcept>

BitRange::BitRange(bool full) : full_(full), empty_(!full), base_(0) {}

BitRange::BitRange(const std::string& range_spec, size_t block_size /*= 100000*/)
    : full_(false), empty_(false), base_(0), range_spec_(range_spec) {
  if (range_spec == "-") {
    empty_ = true;
    return;
  }
  if (range_spec == "+") {
    full_ = true;
    return;
  }
  if (block_size < 1) {
    throw std::runtime_error("invalid block size for DefferedBitset");
  }
  bits_.resize(block_size);
  recompute_block();
}

bool BitRange::operator[](size_t index) const {
  if (full_) {
    return true;
  }

  if (empty_) {
    return false;
  }

  if (index >= base_ + bits_.size() || index < base_) {
    base_ = index;
    recompute_block();
  }

  return bits_[index - base_];
}

// Parses 'range' string of format:
//   range := subrange | subrange,range
//   subrange := a | a-b | a-b:c
void BitRange::recompute_block() const {
  // reset to all false
  std::vector<bool>(bits_.size()).swap(bits_);
  std::stringstream str(range_spec_);

  const size_t min_frame = base_;
  const size_t max_frame = base_ + bits_.size() - 1;

  size_t frame_offset = 0;

  while (str) {
    size_t frame = 0;
    str >> frame;
    frame += frame_offset;

    int next_value = str.peek();
    if (next_value == '-') {
      //Read x-y pattern
      str.get();
      size_t last = 0;
      str >> last;
      last += frame_offset;

      // Allow user to specify range however he likes.
      if (frame > last) {
        std::swap(frame, last);
      }

      last = std::min(last, max_frame);

      next_value = str.peek();
      if (next_value == ':') {
        //Read x-y:z pattern
        str.get();
        size_t every_nth_frame = 1;
        str >> every_nth_frame;

        if (every_nth_frame == 0) {
          throw std::runtime_error("Frame step can't be 0");
        }

        size_t begin_frame = std::max(frame, min_frame);

        size_t steps = (begin_frame - frame) / every_nth_frame;
        if ((begin_frame - frame) % every_nth_frame != 0) {
          steps++;
        }

        frame += every_nth_frame * steps;

        //Write pattern
        for (; frame <= last; frame += every_nth_frame) {
          bits_[frame - base_] = true;
        }
      } else {
        //Write pattern
        frame = std::max(frame, min_frame);
        for (; frame <= last; ++frame) {
          bits_[frame - base_] = true;
        }
      }
    } else {
      //Write pattern
      if (frame >= min_frame && frame <= max_frame) {
        bits_[frame - base_] = true;
      }
    }

    next_value = str.peek();
    if (next_value == '/') {
      //Read .../z pattern
      str.get();
      size_t repeated_range;
      str >> repeated_range;

      str.peek();
      if (!str.eof()) {
        throw std::runtime_error("Unexpected character after pattern repeat count: " + range_spec_);
      }

      frame_offset += repeated_range;
      //Restart pattern
      if (frame_offset < max_frame) {
        str.str(range_spec_);
        str.clear();
      }
    } else if (next_value == ',' || str.eof()) {
      str.get();
    } else {
      throw std::runtime_error("Unexpected character after frame number: " + range_spec_);
    }
  }

  if (str.eof()) {
    return;
  }

  throw std::runtime_error("Couldn't parse whole range string: " + range_spec_);
}

std::istream& operator>>(std::istream& lhs, BitRange& rhs) {
  std::string str;
  lhs >> str;
  rhs = BitRange(str);
  return lhs;
}
