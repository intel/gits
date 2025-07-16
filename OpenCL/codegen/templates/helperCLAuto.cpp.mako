// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "helperCL.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#if defined GITS_API_OCL && !defined CCODE_FOR_EGL

namespace cl {

void CL_CALLBACK CallbackContext(const char*, const void*, size_t, void*) {}
void CL_CALLBACK CallbackProgram(cl_program, void*) {}
void CL_CALLBACK CallbackEvent(cl_event, cl_int, void*) {}
void CL_CALLBACK CallbackMem(cl_mem, void*) {}
cl::TUserData userData;
} // namespace cl

void Load(const std::string& fileName, size_t bufferSize, char* buffer) {
  int fileSize = (int)FileSize(fileName);
  if (fileSize != bufferSize) {
    std::cerr << "Buffer size (" << bufferSize << ") do not match file size (" << fileSize
              << ") of: " << fileName << "\n";
    exit(EXIT_FAILURE);
  }

  std::string filePath = Configurator::Get().common.player.streamDir.string() + fileName;
  std::ifstream file(filePath.c_str(), std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Failed to open file: " << filePath << "\n";
    exit(EXIT_FAILURE);
  }

  file.read(buffer, bufferSize);
  if (!file) {
    std::cerr << "Failed reading: " << filePath << "\n";
    exit(EXIT_FAILURE);
  }
}

void Load(const std::string& fileName, unsigned offset, size_t bufferSize, char* buffer) {
  uint64_t fileSize = FileSize(fileName);
  if (fileSize < bufferSize) {
    std::cerr << "Buffer size is bigger than the file size of: " << fileName << "\n";
    exit(EXIT_FAILURE);
  }
  std::string filePath = Configurator::Get().common.player.streamDir.string() + fileName;
  std::ifstream file(filePath.c_str(), std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Failed to open file: " << filePath << "\n";
    exit(EXIT_FAILURE);
  }

  file.seekg(offset, std::ios::beg);
  file.read(buffer, bufferSize);
  if (!file) {
    std::cerr << "Failed reading: " << filePath << "\n";
    exit(EXIT_FAILURE);
  }
}

void SleepIf(bool exprResult, long milliseconds) {
  if (exprResult) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
  }
}

void CLInit() {
  gits::OpenCL::drvOcl.Initialize();
}

#endif /* GITS_API_OCL */

namespace api {
%for name, func in without_field(functions, 'version').items():
${get_return_type(func)} \
(STDCALL*& ${name}) \
(${make_params(func, with_types=True, one_line=True)}) \
= gits::OpenCL::drvOcl.${name};
%endfor
} // namespace api
