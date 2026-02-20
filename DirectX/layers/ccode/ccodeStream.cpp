// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "ccodeStream.h"
#include "ccodeStringLiterals.h"
#include "ccodeUtilsAuto.h"
#include "to_string/toStr.h"
#include "configurator.h"
#include "gits.h"

#include <filesystem>

namespace gits {
namespace DirectX {
namespace ccode {

CCodeStream::CCodeStream() {
  auto& cfg = Configurator::Get();
  GITS_ASSERT(cfg.directx.player.cCode.enabled);

  // Prepare output directory
  auto outputDir = cfg.common.player.outputDir;
  if (outputDir.empty()) {
    outputDir = cfg.common.player.streamDir;
  }
  std::filesystem::create_directories(outputDir);

  // Copy the empty CCode project to the output directory
  const auto& ccodeProjectDir = cfg.directx.player.cCode.cCodePath;
  GITS_ASSERT(std::filesystem::exists(ccodeProjectDir));
  try {
    std::filesystem::copy(ccodeProjectDir, outputDir / "CCode",
                          std::filesystem::copy_options::recursive |
                              std::filesystem::copy_options::overwrite_existing);
    LOG_INFO << "CCode - Copied empty CCode project to " << outputDir;
  } catch (const std::filesystem::filesystem_error& e) {
    LOG_ERROR << "CCode - Failed to copy CCode directory: " << e.what();
    GITS_ASSERT(false, "CCode - Failed to setup CCode project.");
  }

  // Prepare the output directory for the generated data and source
  generatedDir_ = outputDir / "CCode/generated";
  if (!std::filesystem::create_directory(generatedDir_)) {
    LOG_WARNING << "CCode - CCode has already been generated for stream. Will overwrite "
                   "generated code and data.";
  }

  // Write config.cmake with executable name
  auto appName = CGits::Instance().FilePlayer().GetApplicationName();
  if (appName.ends_with(".exe")) {
    appName.erase(appName.length() - 4);
  }
  std::ofstream cmakeConfig(generatedDir_ / "config.cmake", std::ios::out);
  GITS_ASSERT(cmakeConfig.good());
  cmakeConfig << "set(EXECUTABLE_NAME \"" << appName << "\")" << std::endl;
  cmakeConfig.close();

  // Write out commands.h
  std::ofstream commandsH(generatedDir_ / "commands.h", std::ios::out);
  GITS_ASSERT(commandsH.good());
  commandsH << g_CommandsH;
  commandsH.close();

  // Open objects.h for writing
  objectsH_.open(generatedDir_ / "objects.h", std::ios::out);
  GITS_ASSERT(objectsH_.good());
  objectsH_ << g_ObjectsH;

  // Open the binary data file for writing
  data_.open(generatedDir_ / "data.bin", std::ios_base::binary);
  GITS_ASSERT(data_.good());
}

CCodeStream::~CCodeStream() {
  // Build commands.cpp
  std::ofstream commandsCpp(generatedDir_ / "commands.cpp", std::ios::out);
  GITS_ASSERT(commandsCpp.good());
  commandsCpp << g_CommandsCpp;

  // Forward declaration of all block functions
  for (const auto& block : blocks_) {
    commandsCpp << "void " << block.second << "();" << std::endl;
  }
  commandsCpp << std::endl;

  // StateRestore function
  commandsCpp << "void StateRestore() {" << std::endl;
  commandsCpp << "  LOG_INFO << \"CCode - Restoring state...\";" << std::endl;
  for (const auto& block : blocks_) {
    if (block.first.isStateRestore) {
      commandsCpp << "  " << block.second << "();" << std::endl;
    }
  }
  commandsCpp << "}" << std::endl;

  // Function for each frame (Frame_0, Frame_1, ...)
  unsigned currentFrame = 0;
  commandsCpp << std::endl;
  commandsCpp << "void Frame_0() {" << std::endl;
  commandsCpp << "  LOG_INFO << \"CCode - Frame 0...\";" << std::endl;
  for (const auto& block : blocks_) {
    // Skip state restore blocks
    if (block.first.isStateRestore) {
      continue;
    }
    // New frame block
    if (block.first.frame > currentFrame) {
      commandsCpp << "}" << std::endl;
      commandsCpp << "void Frame_" << block.first.frame << "() {" << std::endl;
      commandsCpp << "  LOG_INFO << \"CCode - Frame " << block.first.frame << "...\";" << std::endl;
      currentFrame = block.first.frame;
    }
    commandsCpp << "  " << block.second << "();" << std::endl;
  }
  commandsCpp << "}" << std::endl;

  // RunFrames function executes all frames
  commandsCpp << std::endl;
  commandsCpp << "void RunFrames() {" << std::endl;
  for (unsigned i = 0; i <= currentFrame; ++i) {
    commandsCpp << "  Frame_" << i << "();" << std::endl;
  }
  commandsCpp << "}" << std::endl;
  commandsCpp.close();
}

std::ostream& CCodeStream::getCurrentBlock() {
  return currentBlock_;
}

std::ostream& CCodeStream::getObjectsHeader() {
  return objectsH_;
}

size_t CCodeStream::writeData(const void* data, size_t size) {
  data_.write(reinterpret_cast<const char*>(data), size);
  dataOffset_ += size;
  return dataOffset_;
}

size_t CCodeStream::getDataOffset() const {
  return dataOffset_;
}

void CCodeStream::addInterface(unsigned key, REFIID iid) {
  auto latestIID = getLatestInterface(iid);
  auto iidStr = toStr(latestIID);
  if (iidStr.starts_with("IID_")) {
    iidStr = iidStr.substr(4);
  }
  interfaceMap_[key] = iidStr;
}

std::string CCodeStream::getInterfaceName(unsigned key) const {
  auto it = interfaceMap_.find(key);
  return (it != interfaceMap_.end()) ? it->second : std::string();
}

void CCodeStream::writeBlock(const BlockInfo& info) {
  static unsigned s_firstCommandIndex = 1;
  static unsigned s_lastCommandIndex = 0;
  s_lastCommandIndex = s_lastCommandIndex + info.commands;

  // Block name based on frame / state restore and command indices
  // e.g. StateRestore_1_1500, Frame_0_1501_3500
  std::ostringstream blockNameSs;
  if (info.isStateRestore) {
    blockNameSs << "StateRestore";
  } else {
    blockNameSs << "Frame_" << info.frame;
  }
  blockNameSs << "_" << s_firstCommandIndex << "_" << s_lastCommandIndex;

  std::string blockName = blockNameSs.str();
  blocks_.push_back({info, blockName});

  auto toLower = [](std::string str) {
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return str;
  };

  // Start a new file for the block
  std::string fileName = "commands_" + toLower(blockName) + ".cpp";
  std::ofstream commandsCpp(generatedDir_ / fileName, std::ios::out);
  GITS_ASSERT(commandsCpp.good());
  commandsCpp << g_CommandsXCpp;
  commandsCpp << "void " << blockName << "() {" << std::endl;
  // Write the current block content to the file
  std::string_view currentBlockContent = currentBlock_.view();
  commandsCpp.write(currentBlockContent.data(), currentBlockContent.size());
  // Close the block function and the file
  commandsCpp << "}" << std::endl;
  commandsCpp.close();

  LOG_INFO << "CCode - Created new block: " << blockName << " (" << info.commands << " commands)";

  // Reset the current block stream for the next block
  currentBlock_ = std::ostringstream();

  s_firstCommandIndex = s_lastCommandIndex;
}

CCodeStream& CCodeStream::getInstance() {
  static CCodeStream instance;
  return instance;
}

} // namespace ccode
} // namespace DirectX
} // namespace gits
