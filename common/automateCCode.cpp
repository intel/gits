// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "automateCCode.h"
#include <algorithm>
#include <sstream>
#include <fstream>
#include <iomanip>

namespace gits {

std::map<int, std::string> scopeAnalyze(const std::filesystem::path& path) {
  std::map<int, std::string> breaks;
  int fileDivider = 1048576;
  int nestLevel = 0;
  int lineNo = 1;
  std::string prevLine = "";
  std::string currentFunction = "";
  int prevLevel = 0;
  bool needsBreak = false;
  std::ifstream file(path);
  std::string line;
  auto startPosition = file.tellg();
  while (std::getline(file, line)) {
    auto endPosition = file.tellg();
    if (endPosition - startPosition >= fileDivider) {
      needsBreak = true;
      startPosition = endPosition;
    }

    size_t commentPos = line.find("//");
    std::string lineTrimmed;
    if (commentPos != std::string::npos) {
      lineTrimmed = line.substr(0, commentPos + 1);
    } else {
      lineTrimmed = line;
    }

    int opens = std::count(lineTrimmed.begin(), lineTrimmed.end(), '{');
    int closes = std::count(lineTrimmed.begin(), lineTrimmed.end(), '}');
    int balance = opens - closes;

    if (balance != 0) {
      prevLevel = nestLevel;
      nestLevel += balance;

      if (prevLevel == 0 && nestLevel == 1) {
        currentFunction = prevLine.substr(5, prevLine.size() - 7);
      }
    }
    if (needsBreak && nestLevel == 2 && opens == 1 && closes == 0) {
      breaks.insert({lineNo - 1, currentFunction});
      needsBreak = false;
    }
    prevLine = line;
    lineNo++;
  }
  return breaks;
}

std::map<std::string, std::vector<int>> calculateContinuations(
    const std::map<int, std::string>& breakPoints) {
  std::map<std::string, std::vector<int>> continuations;
  int it = 0;
  for (const auto& breakPoint : breakPoints) {
    continuations[breakPoint.second].push_back(it++);
  }
  return continuations;
}

std::string getOutputFileName(const std::filesystem::path& path, int num) {
  std::stringstream outputFileName;
  outputFileName << path.stem().string() << std::setfill('0') << std::setw(6) << std::dec << num
                 << ".cpp";
  return outputFileName.str();
}

void divideFile(const std::filesystem::path& path,
                const std::filesystem::path& outputPath,
                std::map<int, std::string>& breakPoints) {
  auto continuations = calculateContinuations(breakPoints);
  int currentLine = 1;
  int currentFile = 1;
  std::filesystem::path outputFilePath = outputPath / getOutputFileName(path, currentFile);
  std::ofstream outputFile(outputFilePath);
  std::string prevFunction = "";

  std::ifstream inputFile(path);
  std::string line;
  while (std::getline(inputFile, line)) {
    outputFile << line << "\n";
    if (!breakPoints.empty() && currentLine == breakPoints.begin()->first) {
      auto breakPoint = breakPoints.begin();
      currentFile++;

      if (breakPoint->second != prevFunction) {
        for (const auto& continuation : continuations[breakPoint->second]) {
          outputFile << "  void " << breakPoint->second << "_cont_" << continuation + 2 << "();\n";
          outputFile << "  " << breakPoint->second << "_cont_" << continuation + 2 << "();\n";
        }
      }

      prevFunction = breakPoint->second;

      outputFile << "}\n";
      outputFile.close();
      outputFilePath = outputPath / getOutputFileName(path, currentFile);
      outputFile = std::ofstream(outputFilePath);
      outputFile << "#include \"gitsApi.h\"\n";
      outputFile << "#include \"stream_externs.h\"\n";
      outputFile << "#include \"helperVk.h\"\n";
      outputFile << "#include \"helperCL.h\"\n";
      outputFile << "#include \"helperL0.h\"\n";
      outputFile << "#include \"helperGL.h\"\n";
      outputFile << "void " << breakPoint->second << "_cont_" << currentFile << "()\n";
      outputFile << "{\n";
      breakPoints.erase(breakPoint->first);
    }
    currentLine++;
  }
}
} // namespace gits
