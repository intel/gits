// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <filesystem>
#include <fstream>

class DataService {
public:
  static DataService& Get();

  bool Open(const std::filesystem::path& dataFile);
  bool Read(void* outBuffer, size_t size);

private:
  DataService() = default;
  ~DataService();

  // Prevent copying and assignment
  DataService(const DataService&) = delete;
  DataService& operator=(const DataService&) = delete;

  std::ifstream m_FileStream;
};
