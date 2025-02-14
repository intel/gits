// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#ifdef GITS_PLUGIN_DLL
#ifdef _WIN32
#ifdef GITS_PLUGIN_EXPORT_API
#define GITS_PLUGIN_API __declspec(dllexport)
#else
#define GITS_PLUGIN_API __declspec(dllimport)
#endif
#else
#define GITS_PLUGIN_API __attribute__((visibility("default")))
#endif
#else
#define GITS_PLUGIN_API
#endif

#include <string>
#include <assert.h>

namespace gits {
class CGits;
} // namespace gits

class IPlugin {
public:
  IPlugin(gits::CGits& gits, const char* pluginPath){};
  virtual ~IPlugin() = default;

  virtual const char* getName() = 0;
  virtual void* getImpl() = 0;
};

// Plugin DLL exports

extern "C" {
using CreatePluginPtr = IPlugin* (*)(gits::CGits&, const char*);
GITS_PLUGIN_API IPlugin* createPlugin(gits::CGits& gits, const char* pluginPath);
using DestroyPluginPtr = void* (*)();
GITS_PLUGIN_API void destroyPlugin();
}
