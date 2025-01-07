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
#include <cstdint>

namespace gits {
class Ar {
  struct ArSpecialCases {
    static constexpr std::string_view arFileEntryTrailingMagic = "\x60\x0A";
    static constexpr std::string_view longFileNamesFile = "//";
    static constexpr char longFileNamePrefix = '/';
    static constexpr char fileNameTerminator = '/';
  };

  struct ArFileEntryHeader {
    char identifier[16] = {'/', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
                           ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
    char fileModificationTimestamp[12] = {'0', ' ', ' ', ' ', ' ', ' ',
                                          ' ', ' ', ' ', ' ', ' ', ' '};
    char ownerId[6] = {'0', ' ', ' ', ' ', ' ', ' '};
    char groupId[6] = {'0', ' ', ' ', ' ', ' ', ' '};
    char fileMode[8] = {'6', '4', '4', ' ', ' ', ' ', ' ', ' '};
    char fileSizeInBytes[10] = {'0', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
    char trailingMagic[2] = {ArSpecialCases::arFileEntryTrailingMagic[0],
                             ArSpecialCases::arFileEntryTrailingMagic[1]};
  };
  static_assert(60U == sizeof(ArFileEntryHeader), "Wrong size detected in ArFileEntryHeader");

  struct ArFileData {
    std::string fileName;
    std::vector<uint8_t> fileData;
  };

  ArFileData longFileNamesEntry;

  bool IsStringPadding(char character);
  std::string ReadUnpaddedString(const char* paddedString, size_t maxLength);
  std::string ReadLongFileName(std::string longFileNamesSection, size_t offset);

public:
  static constexpr std::string_view arMagic = "!<arch>\n";
  static constexpr std::string_view paddingFilenamePrefix = "pad_";

  std::vector<ArFileData> files;

  Ar(const uint8_t* data, size_t dataLength);
};
} // namespace gits
