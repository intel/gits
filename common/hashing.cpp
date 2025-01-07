// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#ifndef BUILD_FOR_CCODE

#include "xxhash.h"
#include "log.h"
#include "tools.h"

#include <regex>
#include <fstream>

namespace gits {

static const unsigned int INPUT_BUFFER_SIZE = 4096;
static const unsigned int XXHASH_DIGEST_SIZE = 4;
static const unsigned int NEWEST_SIG_FORMAT_VERSION = 1;
static const std::string FFV_STRING = "File format version: ";
static const std::string SIG_FILE_NAME = "gitsSignature.hash";
static const std::string OLD_SIG_FILE_NAME = "gitsSignature.md5";
static const std::vector<std::regex> SKIPPED_FILES = {
    std::regex(".*\\.(hash|md5)"),                   // old and new signature files
    std::regex("(gits)?Screenshots/.*"),             // contents of GITS screenshot directories
    std::regex(".*gitsPlayer"),                      // gitsPlayer executable
    std::regex(".*CCodeProject"),                    // compiled CCode - executable
    std::regex("\\.DS_Store|desktop\\.ini"),         // OS-specific metadata
    std::regex(".*~"),                               // unix backup files (e.g. "something~")
    std::regex(".*\\.(h|cpp|sw[mnop]|bak|ini|csv)"), // various file extensions
};

std::string file_xxhash(const std::filesystem::path& filename) {
  FILE* file = fopen(filename.string().c_str(), "rb");
  if (file == nullptr) {
    throw std::runtime_error("Failed to open file for checksum calculation: " + filename.string());
  }

  static uint8_t buffer[INPUT_BUFFER_SIZE];
  size_t bytesRead;
  XXH32_state_t* const ctx = XXH32_createState();
  if (ctx == nullptr) {
    fclose(file);
    const auto msg = "Initialization of XXH32 failed.";
    throw std::runtime_error(std::string(EXCEPTION_MESSAGE) + msg);
  }
  XXH32_reset(ctx, 0);
  while ((bytesRead = fread(buffer, sizeof(uint8_t), sizeof(buffer), file)) != 0) {
    XXH32_update(ctx, buffer, bytesRead);
  }
  fclose(file);

  const XXH32_hash_t hash = XXH32_digest(ctx);
  std::string result = bytesToHex((uint8_t*)&hash, XXHASH_DIGEST_SIZE);

  XXH32_freeState(ctx);

  return result;
}

struct Signature {
  unsigned int fileFormatVersion;
  std::map<std::string, std::string> hashes;
};

static Signature calculateSig(const std::filesystem::path& dir) {
  std::map<std::string, std::string> hashes;
  for (const auto& dirEntry : std::filesystem::directory_iterator(dir)) {
    const std::string relPathStr = std::filesystem::relative(dirEntry, dir).string();
    if (dirEntry.is_regular_file()) {
      bool excluded = false;
      for (const std::regex& exclude : SKIPPED_FILES) {
        if (std::regex_match(relPathStr, exclude)) {
          excluded = true;
          break;
        }
      }

      if (!excluded) {
        hashes[relPathStr] = file_xxhash(dirEntry);
      }
    }
  }

  return Signature{NEWEST_SIG_FORMAT_VERSION, std::move(hashes)};
}

static Signature readSigFromDir(const std::filesystem::path& dir) {
  unsigned int version;
  std::map<std::string, std::string> hashes;
  std::string line;

  // Find the newest signature file.
  std::filesystem::path filepath = dir / SIG_FILE_NAME;
  std::filesystem::path oldFilepath = dir / OLD_SIG_FILE_NAME;
  if (!std::filesystem::exists(filepath)) {
    if (!std::filesystem::exists(oldFilepath)) {
      Log(ERR) << "Signature file not found. This usually means the game/app "
                  "process was terminated before recorder could finish writing the "
                  "stream to disk. Possible fixes and workarounds are described in "
                  "User Guide section \"Streams are not being finished properly\".";
      throw std::runtime_error("Signature file not found.");
    } else {
      filepath = std::move(oldFilepath);
    }
  }

  // Open the file.
  std::ifstream sigFile(filepath);
  if (!sigFile.is_open()) {
    throw std::runtime_error("Couldn't open the signature file: " + filepath.string());
  }

  // Read a version if there is one.
  uniGetLine(sigFile, line);
  auto verPos = line.find(FFV_STRING);
  if (verPos != std::string::npos) {
    version = std::stoi(line.substr(verPos + FFV_STRING.length()));
  } else {
    version = 0;                     // No version means that was the legacy format.
    sigFile.seekg(0, std::ios::beg); // Going back one short line should be cheap.
  }

  // Read the hashes.
  while (uniGetLine(sigFile, line)) {
    auto spacePos = line.find(' ');
    if (spacePos == std::string::npos) {
      throw std::runtime_error("Signature file is malformed.");
    }
    hashes[line.substr(spacePos + 1)] = line.substr(0, spacePos);
  }

  return Signature{version, std::move(hashes)};
}

void sign_directory(const std::filesystem::path& dir) {
  bool isDir = std::filesystem::is_directory(dir);
  bool isSymlinkToDir = std::filesystem::is_symlink(dir) &&
                        std::filesystem::is_directory(std::filesystem::read_symlink(dir));
  if (!isDir && !isSymlinkToDir) {
    throw std::runtime_error("Can't generate the signature file, path is not a directory: " +
                             dir.string());
  }

  const Signature sig = calculateSig(dir);
  std::filesystem::path filepath = dir / SIG_FILE_NAME;
  std::ofstream sigFile(filepath, std::ios::out | std::ios::binary);
  if (!sigFile.is_open()) {
    throw std::runtime_error("Couldn't create a file with signatures: " + filepath.string());
  }

  sigFile << FFV_STRING << sig.fileFormatVersion << "\n";
  for (const auto& kv : sig.hashes) {
    sigFile << kv.second << " " << kv.first << "\n";
  }
}

void verify_directory(const std::filesystem::path& dir) {
  Signature sig = readSigFromDir(dir);

  Log(INFO) << "Signature file format version " << sig.fileFormatVersion;
  switch (sig.fileFormatVersion) {
  case 1:
    for (const auto& kv : sig.hashes) {
      try {
        std::string digest = file_xxhash(dir / kv.first);
        if (caseInsensitiveEquals(kv.second, digest)) {
          Log(INFO) << "Stream file: " << kv.first << " has an expected hash value.";
        } else {
          Log(WARN) << "Stream integrity violated. File " << kv.first
                    << " has an unexpected hash value.";
        }
      } catch (const std::exception& ex) {
        Log(WARN) << "Stream integrity possibly violated. Exception: " << ex.what();
      }
    }
    break;
  case 0: {
    Log(WARN) << "MD5 signature format is deprecated. Use older version to check the signature. "
                 "Skipping integrity check.";
    break;
  }
  default:
    Log(WARN) << "Stream integrity unchecked. The signatures file format version is too high.";
  }
}

} // namespace gits
#endif
