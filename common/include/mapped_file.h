// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "pragmas.h"

#include <memory>
#include <cstdint>

namespace boost {
namespace interprocess {
class mapped_region;
class file_mapping;
} // namespace interprocess
} // namespace boost

namespace gits {
using boost::interprocess::file_mapping;

class mapped_file {
public:
  mapped_file();
  mapped_file(file_mapping& mapping);
  mapped_file(file_mapping& mapping, uint64_t offset, size_t size);
  const char* address() const;
  size_t size() const;
  void page_in() const;
  void swap(mapped_file& other);

private:
  std::shared_ptr<boost::interprocess::mapped_region> region_;
};
} // namespace gits
