// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "mapped_file.h"
#include "pragmas.h"

#include <map>

DISABLE_WARNINGS
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
ENABLE_WARNINGS

namespace gits {
mapped_file::mapped_file() {}

mapped_file::mapped_file(file_mapping& mapping) {
  using namespace boost::interprocess;
  region_.reset(new mapped_region(mapping, read_only));
}

mapped_file::mapped_file(file_mapping& mapping, uint64_t offset, size_t size) {
  using namespace boost::interprocess;
  region_.reset(new mapped_region(mapping, read_only, offset, size));
}

const char* mapped_file::address() const {
  if (region_) {
    return static_cast<char*>(region_->get_address());
  }
  return nullptr;
}

size_t mapped_file::size() const {
  if (region_) {
    return region_->get_size();
  }
  return 0;
}

void mapped_file::page_in() const {
  size_t size = this->size();
  const char* ptr = address();
  for (size_t i = 0; i < size; i += region_->get_page_size()) {
    volatile char datum = ptr[i];
    (void)datum;
  }
}

void mapped_file::swap(mapped_file& other) {
  region_.swap(other.region_);
}
} // namespace gits
