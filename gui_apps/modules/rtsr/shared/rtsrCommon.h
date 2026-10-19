// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <cstddef>
#include <cstdint>

// Shared between the module's ui and its gitsPlugins, hence outside the gui
// namespace: the plugins are not GUI code.
namespace gits::rtsr {

// Reported by the plugin's IPlugin::getName() and added by the host to the GITS
// config plugin list of every run it launches while the module is enabled. Must
// also match Info.Name in gitsPlugins/DirectX/rtsr/config.yml, which the Plugins
// panel compares against the config list verbatim.
constexpr const char* PLUGIN_NAME = "rtsr";

// TODO These duplicate the SideBarItem::RTSR entry in the launcher's labels.h.
// The duplication goes away once the host application reads module navigation
// text through ModuleBasePanel::GetName()/GetNameHint() instead of hardcoding
// every module panel.
constexpr const char* MODULE_NAME = "RTSR";
constexpr const char* MODULE_NAME_HINT = "Real-time shader replacement module";

// The stable identity used for harvested and replacement shader blobs.
uint64_t ComputeBytecodeHash(const void* bytecode, size_t size);

} // namespace gits::rtsr
