// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <string>
#include <unordered_set>

namespace gits {
namespace DirectX {

class CommandKeyService {
public:
  CommandKeyService(const std::string& atCommandKey);
  unsigned atCommand(unsigned commandKey);
  std::string keyToString(unsigned key) const;

private:
  void extractKeys(const std::string& keyString, std::unordered_set<unsigned>& keySet);

private:
  std::unordered_set<unsigned> commandKeys_;
};

} // namespace DirectX
} // namespace gits
