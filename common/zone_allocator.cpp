// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "zone_allocator.h"
#include <cassert>
#include <cstdlib>
#include "config.h"

zone_allocator::zone_allocator() : curr_alloc_zone_(0), zone_size_(0) {}

zone_allocator::zone_allocator(size_t zones, size_t zone_size) {
  reinitialize(zones, zone_size);
}

void zone_allocator::reinitialize(size_t zones, size_t zone_size) {
  zone_size_ = zone_size;
  zones_.resize(zones);
  for (auto& z : zones_) {
    z.address.reset(new char[zone_size]);
    z.used = 0;
  }
  this->curr_alloc_zone_ = 0;
}

void* zone_allocator::allocate(size_t size) {
  auto& zone = zones_[curr_alloc_zone_];
  auto address = zone.address.get() + zone.used;
  zone.used += size;
  if (zone.used > zone_size_) {
    throw std::runtime_error("zone size exceeded");
  }
  return address;
}

void zone_allocator::use_next_zone() {
  curr_alloc_zone_++;
  if (curr_alloc_zone_ >= (int)zones_.size()) {
    curr_alloc_zone_ = 0;
  }
  zones_[curr_alloc_zone_].used = 0;
}
