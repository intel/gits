// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <filesystem>
#include <string>

namespace gits {

inline constexpr char STREAM_FILE_EXTENSION[] = ".gits";
inline constexpr char STREAM_FILE_NAME[] = "stream.gits";
// Streams recorded before GITS 3 use these names and must stay playable.
inline constexpr char GITS2_STREAM_FILE_EXTENSION[] = ".gits2";
inline constexpr char GITS2_STREAM_FILE_NAME[] = "stream.gits2";

inline bool IsStreamFile(const std::filesystem::path& path) {
  const std::string name = path.filename().string();
  for (const std::string extension : {STREAM_FILE_EXTENSION, GITS2_STREAM_FILE_EXTENSION}) {
    if (name.ends_with(extension) || name.ends_with(extension + ".gz")) {
      return true;
    }
  }
  return false;
}

// Returns the stream.gits path unless the directory holds only a stream.gits2.
inline std::filesystem::path StreamFileInDirectory(const std::filesystem::path& directory) {
  const auto gits2Stream = directory / GITS2_STREAM_FILE_NAME;
  if (!std::filesystem::exists(directory / STREAM_FILE_NAME) &&
      std::filesystem::exists(gits2Stream)) {
    return gits2Stream;
  }
  return directory / STREAM_FILE_NAME;
}

} // namespace gits
