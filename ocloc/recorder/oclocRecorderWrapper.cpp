// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "oclocRecorderWrapper.h"

#include "gits.h"
#include "log.h"
#include "recorder.h"
#include "tools.h"

#include "oclocDrivers.h"
#include "oclocLibrary.h"
#include "oclocFunctions.h"
#include "oclocStateDynamic.h"
#include "oclocStateTracking.h"
#include "opengl_apis_iface.h"

#include <algorithm>
#include <fstream>
#include <regex>
#include <string>
#include <vector>
#include <set>
#include <filesystem>

static gits::ocloc::CRecorderWrapper* wrapper = nullptr;

gits::ocloc::IRecorderWrapper* STDCALL GITSRecorderOcloc() {
  if (wrapper == nullptr) {
    try {
      // library not set - perform initialization
      // apis_iface.h:127 if(_3d.get() == 0) throw
      gits::CGits::Instance().apis.UseApi3dIface(std::make_shared<gits::OpenGL::OpenGLApi>());
      gits::CRecorder& recorder = gits::CRecorder::Instance();
      wrapper = new gits::ocloc::CRecorderWrapper(recorder);
      recorder.Register(std::make_shared<gits::ocloc::CLibrary>());
    } catch (const std::exception& ex) {
      LOG_ERROR << "Cannot initialize recorder: " << ex.what() << std::endl;
      exit(EXIT_FAILURE);
    }
  }
  return wrapper;
}

// TODO: move most of the functions to the common logic
namespace {
bool CheckIfStringExists(const std::vector<const char*>& vecOfStrings, const std::string& str) {
  for (auto strCheck : vecOfStrings) {
    if (strCheck != nullptr && str.find(strCheck) != std::string::npos) {
      return true;
    }
  }
  return false;
}
bool CheckIfSourceNameExists(const char* fileName,
                             const char** sourcesNames,
                             const uint32_t& numSources) {
  for (uint32_t i = 0; i < numSources; i++) {
    if (std::strcmp(fileName, sourcesNames[i]) == 0) {
      return true;
    }
  }
  return false;
}
} // namespace
namespace gits {
namespace ocloc {
CRecorderWrapper::CRecorderWrapper(CRecorder& recorder) : _recorder(recorder) {}

void CRecorderWrapper::StreamFinishedEvent(std::function<void()> event) {
  _recorder.RegisterDisposeEvent(std::move(event));
}

void CRecorderWrapper::CloseRecorderIfRequired() {
  if (_recorder.IsMarkedForDeletion()) {
    _recorder.Close();
  }
}

void CRecorderWrapper::MarkRecorderForDeletion() {
  if (_recorder.Running() && _recorder.InstancePtr() != nullptr) {
    _recorder.MarkForDeletion();
  }
}

std::recursive_mutex& CRecorderWrapper::GetInterceptorMutex() const {
  return _recorder.GetMutex();
}

CDriver& CRecorderWrapper::Drivers() const {
  return drv;
}

void CRecorderWrapper::InitializeDriver() const {
  drv.Initialize();
}

void CRecorderWrapper::oclocInvoke(int return_value,
                                   unsigned int argc,
                                   const char** argv,
                                   const uint32_t numSources,
                                   const uint8_t** sources,
                                   const uint64_t* sourceLens,
                                   const char** sourcesNames,
                                   const uint32_t numInputHeaders,
                                   const uint8_t** dataInputHeaders,
                                   const uint64_t* lenInputHeaders,
                                   const char** nameInputHeaders,
                                   uint32_t* numOutputs,
                                   uint8_t*** dataOutputs,
                                   uint64_t** lenOutputs,
                                   char*** nameOutputs) const {
  CFunction* _token = nullptr;
  if (_recorder.Running()) {
    std::vector<std::string> arguments;
    std::vector<std::string> files;
    std::vector<std::string> includePaths;
    std::vector<std::string> sourceFilesToScan;
    for (unsigned int i = 0; i < argc; ++i) {
      if (std::string(argv[i]) == "-file" && i + 1 < argc) {
        if (numSources == 0) {
          arguments.push_back(argv[i]);
          arguments.push_back(argv[i + 1]);
        }
        if (CheckIfSourceNameExists(argv[i + 1], sourcesNames, numSources)) {
          arguments.push_back(argv[i]);
          arguments.push_back(argv[i + 1]);
        } else {
          files.push_back(argv[i + 1]);
        }
        sourceFilesToScan.push_back(argv[++i]);
      } else if (std::string(argv[i]) == "-options" && i + 1 < argc) {
        includePaths = GetIncludePaths(argv[i + 1]);
        arguments.push_back(argv[i]);
        arguments.push_back(argv[++i]);
      } else {
        arguments.push_back(argv[i]);
      }
    }
    std::set<std::string> alreadyCreatedHeaders;
    for (auto i = 0U; i < numSources; i++) {
      const auto src = std::string(sources[i], sources[i] + sourceLens[i] - 1);
      const auto headerFiles =
          GetStringsWithRegex(std::move(src), R"(#include\s*["<]([^">]+))", "#include\\s*[<\"]*");
      if (!headerFiles.empty()) {
        CreateHeaderFiles(headerFiles, includePaths, alreadyCreatedHeaders, true);
      }
    }
    CreateHeaderFiles(sourceFilesToScan, includePaths, alreadyCreatedHeaders);

    if (files.size() > 0) {
      const uint32_t newNumSources = numSources + files.size();
      std::vector<const uint8_t*> newSources(newNumSources);
      std::vector<uint64_t> newSourceLens(newNumSources);
      std::vector<const char*> newSourceNames(newNumSources);
      std::vector<std::vector<char>> srcHeaderNames(newNumSources);
      std::vector<std::vector<uint8_t>> srcHeaders(files.size());
      uint32_t i = 0;
      for (; i < numSources; ++i) {
        newSources[i] = sources[i];
        newSourceLens.at(i) = sourceLens[i];
        std::string filePath = std::filesystem::path(sourcesNames[i]).filename().string();
        srcHeaderNames[i].reserve(filePath.size());
        std::copy_n(filePath.c_str(), filePath.size(), srcHeaderNames[i].data());
        srcHeaderNames[i].push_back('\0');
        newSourceNames[i] = srcHeaderNames[i].data();
      }
      for (const auto& fileName : files) {
        if (CheckIfStringExists(newSourceNames, fileName)) {
          continue;
        }
        std::ifstream file(fileName, std::ios::in | std::ios::binary);
        if (!file.good()) {
          throw EOperationFailed(EXCEPTION_MESSAGE + std::string("\nCould not open: ") + fileName);
        }
        file.seekg(0, std::ios::end);
        const auto fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<uint8_t> src(fileSize);
        file.read((char*)src.data(), fileSize);
        file.close();
        src.push_back('\0');
        const auto index = i - numSources;
        srcHeaders[index].reserve(src.size());
        std::copy_n(src.data(), src.size(), srcHeaders[index].data());
        newSources[i] = srcHeaders[index].data();
        newSourceLens[i] = src.size();
        srcHeaderNames[i].reserve(fileName.size());
        std::copy_n(fileName.data(), fileName.size(), std::back_inserter(srcHeaderNames[i]));
        srcHeaderNames[i].push_back('\0');
        newSourceNames[i] = srcHeaderNames[i].data();
        i++;
      }
      std::vector<const char*> args;
      std::transform(arguments.begin(), arguments.end(), std::back_inserter(args),
                     [](auto& str) { return str.c_str(); });
      _token = new CoclocInvoke_V1(return_value, args.size(), args.data(), i, newSources.data(),
                                   newSourceLens.data(), newSourceNames.data(), numInputHeaders,
                                   dataInputHeaders, lenInputHeaders, nameInputHeaders, numOutputs,
                                   dataOutputs, lenOutputs, nameOutputs);
      oclocInvoke_SD(_token, return_value, args.size(), args.data(), i, newSources.data(),
                     newSourceLens.data(), newSourceNames.data(), numInputHeaders, dataInputHeaders,
                     lenInputHeaders, nameInputHeaders, numOutputs, dataOutputs, lenOutputs,
                     nameOutputs);
      _recorder.Schedule(_token);
      return;
    } else {
      _token =
          new CoclocInvoke_V1(return_value, argc, argv, numSources, sources, sourceLens,
                              sourcesNames, numInputHeaders, dataInputHeaders, lenInputHeaders,
                              nameInputHeaders, numOutputs, dataOutputs, lenOutputs, nameOutputs);
    }
    _recorder.Schedule(_token);
  }
  oclocInvoke_SD(_token, return_value, argc, argv, numSources, sources, sourceLens, sourcesNames,
                 numInputHeaders, dataInputHeaders, lenInputHeaders, nameInputHeaders, numOutputs,
                 dataOutputs, lenOutputs, nameOutputs);
}

void CRecorderWrapper::oclocFreeOutput(int return_value,
                                       uint32_t* numOutputs,
                                       uint8_t*** dataOutputs,
                                       uint64_t** lenOutputs,
                                       char*** nameOutputs) const {}
} // namespace ocloc
} // namespace gits
