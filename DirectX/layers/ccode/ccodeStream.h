// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <guiddef.h>
#include <unordered_map>
#include <filesystem>
#include <ostream>
#include <fstream>
#include <string>

namespace gits {
namespace DirectX {
namespace ccode {

class CCodeStream {
public:
  static CCodeStream& getInstance();
  struct BlockInfo {
    bool isStateRestore{false};
    unsigned frame{0};
    unsigned commands{0};
  };

private:
  CCodeStream();
  ~CCodeStream();
  CCodeStream(const CCodeStream&) = delete;
  CCodeStream& operator=(const CCodeStream&) = delete;

public:
  std::ostream& getCurrentBlock();
  std::ostream& getObjectsHeader();

  size_t writeData(const void* data, size_t size);
  size_t getDataOffset() const;

  void addInterface(unsigned key, REFIID iid);
  std::string getInterfaceName(unsigned key) const;

  void writeBlock(const BlockInfo& info);

private:
  std::filesystem::path generatedDir_ = {};
  std::vector<std::pair<BlockInfo, std::string>> blocks_{};
  std::unordered_map<unsigned, std::string> interfaceMap_;

  // Open streams
  std::ofstream objectsH_;
  std::ostringstream currentBlock_;
  std::ofstream data_;
  size_t dataOffset_ = 0;
};

} // namespace ccode
} // namespace DirectX
} // namespace gits
