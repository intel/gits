// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <map>
#include <vector>
#include <deque>
#include <cstdlib>
#include <stdint.h>
#include <memory>

struct zone {
  std::shared_ptr<char> address;
  size_t used;
  int padding[16];
};

class zone_allocator {
public:
  zone_allocator();
  zone_allocator(size_t zones, size_t zone_size);
  void* allocate(size_t size);
  void use_next_zone();
  void reinitialize(size_t zones, size_t zone_size);

private:
  int curr_alloc_zone_;
  size_t zone_size_;
  std::vector<zone> zones_;
};
