// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "dataService.h"

#include <plog/Log.h>
#include <iostream>

DataService& DataService::Get() {
  static DataService instance;
  return instance;
}

bool DataService::Open(const std::filesystem::path& dataFile) {
  // Close any previously opened file
  if (m_FileStream.is_open()) {
    m_FileStream.close();
  }

  // Open the new file
  m_FileStream.open(dataFile, std::ios::in | std::ios::binary);
  return m_FileStream.is_open();
}

bool DataService::Read(void* outBuffer, size_t size) {
  if (!m_FileStream.is_open()) {
    LOG_ERROR << "CCode - File not open for reading.";
    return false;
  }

  m_FileStream.read(static_cast<char*>(outBuffer), size);
  if (m_FileStream.gcount() != static_cast<std::streamsize>(size)) {
    LOG_ERROR << "CCode - Failed to read the requested amount of data.";
    return false;
  }

  return true;
}

DataService::~DataService() {
  if (m_FileStream.is_open()) {
    m_FileStream.close();
  }
}
