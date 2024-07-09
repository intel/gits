// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "arDecoder.h"

#include "log.h"
#include "exception.h"

namespace gits {
bool Ar::IsStringPadding(char character) {
  switch (character) {
  case ' ':
  case '\0':
  case '/':
    return true;
  }
  return false;
}

std::string Ar::ReadUnpaddedString(const char* paddedString, size_t maxLength) {
  size_t unpaddedSize = maxLength - 1;
  for (; unpaddedSize > 0U; --unpaddedSize) {
    if (IsStringPadding(paddedString[unpaddedSize]) == false) {
      break;
    }
  }
  if (IsStringPadding(paddedString[unpaddedSize]) == false) {
    ++unpaddedSize;
  }
  return std::string(paddedString, unpaddedSize);
}

std::string Ar::ReadLongFileName(std::string longFileNamesSection, size_t offset) {
  size_t end = offset;
  while ((end < longFileNamesSection.size()) &&
         (longFileNamesSection[end] != ArSpecialCases::fileNameTerminator)) {
    ++end;
  }
  return std::string(longFileNamesSection.data() + offset, end - offset);
}

Ar::Ar(const uint8_t* data, size_t dataLength) {
  std::vector<uint8_t> binary(data, data + dataLength);
  const uint8_t* decodePos = binary.data() + arMagic.size();
  while (decodePos + sizeof(ArFileEntryHeader) <= &binary.back()) {
    auto fileEntryHeader = reinterpret_cast<const ArFileEntryHeader*>(decodePos);
    auto fileEntryDataPos = decodePos + sizeof(ArFileEntryHeader);
    const auto fileSize = std::stoull(fileEntryHeader->fileSizeInBytes);
    if (fileSize + (fileEntryDataPos - binary.data()) > binary.size()) {
      Log(ERR) << "Corrupt AR archive - out of bounds data of file entry with idenfitier '" +
                      std::string(fileEntryHeader->identifier,
                                  sizeof(fileEntryHeader->identifier)) +
                      "'";
      throw EOperationFailed(EXCEPTION_MESSAGE);
    }

    if (std::string(fileEntryHeader->trailingMagic, sizeof(fileEntryHeader->trailingMagic)) !=
        ArSpecialCases::arFileEntryTrailingMagic) {
      Log(WARN) << "File entry header with identifier '" +
                       std::string(fileEntryHeader->identifier,
                                   sizeof(fileEntryHeader->identifier)) +
                       "' has invalid header trailing string";
    }

    ArFileData fileEntry = {};
    fileEntry.fileName =
        ReadUnpaddedString(fileEntryHeader->identifier, sizeof(fileEntryHeader->identifier));
    fileEntry.fileData = std::vector<uint8_t>(fileEntryDataPos, fileEntryDataPos + fileSize);

    if (fileEntry.fileName.empty()) {
      if (ArSpecialCases::longFileNamesFile == std::string(fileEntryHeader->identifier, 2U)) {
        fileEntry.fileName = ArSpecialCases::longFileNamesFile;
        longFileNamesEntry = std::move(fileEntry);
      } else {
        Log(ERR) << "Corrupt AR archive - file entry does not have identifier : '" +
                        std::string(fileEntryHeader->identifier,
                                    sizeof(fileEntryHeader->identifier)) +
                        "'";
        throw EOperationFailed(EXCEPTION_MESSAGE);
      }
    } else {
      if (ArSpecialCases::longFileNamePrefix == fileEntry.fileName[0]) {
        const auto longFileNamePos = std::stoull(fileEntryHeader->identifier + 1);
        fileEntry.fileName = ReadLongFileName(
            std::string(reinterpret_cast<const char*>(longFileNamesEntry.fileData.data()),
                        longFileNamesEntry.fileData.size()),
            static_cast<size_t>(longFileNamePos));
        if (fileEntry.fileName.empty()) {
          Log(ERR) << "Corrupt AR archive - long file name entry has broken identifier : '" +
                          std::string(fileEntryHeader->identifier,
                                      sizeof(fileEntryHeader->identifier)) +
                          "'";
          throw EOperationFailed(EXCEPTION_MESSAGE);
        }
      }
      if (fileEntry.fileName.size() < paddingFilenamePrefix.size() ||
          fileEntry.fileName.substr(0, paddingFilenamePrefix.size()) != paddingFilenamePrefix) {
        files.push_back(std::move(fileEntry));
      }
    }

    decodePos = fileEntryDataPos + fileSize;
    decodePos += fileSize & 1U; // implicit 2-byte alignment
  }
}
} // namespace gits
