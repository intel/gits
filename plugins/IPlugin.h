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
#include <cassert>

namespace gits {
class CGits;
class MessageBus;
struct Configuration;
} // namespace gits

namespace plog {
class IAppender;
} // namespace plog

struct IPluginContext {
  const gits::Configuration* config;
  gits::CGits* gits;
  gits::MessageBus* msgBus;
  plog::IAppender* logAppender;
};

class IPlugin {
public:
  IPlugin(IPluginContext context, const char* pluginPath){};
  virtual ~IPlugin() = default;

  virtual const char* getName() = 0;
  virtual void* getImpl() = 0;
};

// Plugin DLL exports

extern "C" {
using CreatePluginPtr = IPlugin* (*)(IPluginContext, const char*);
GITS_PLUGIN_API IPlugin* createPlugin(IPluginContext context, const char* pluginPath);
using DestroyPluginPtr = void* (*)();
GITS_PLUGIN_API void destroyPlugin();
}
