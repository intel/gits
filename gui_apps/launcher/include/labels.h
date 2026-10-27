// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <map>
#include <string>
#include <vector>
#include <algorithm>

#include "imGuiHelper.h"
#include "baseCommon.h"
#include "baseContext.h"
#include "context.h"
#include "baseLabels.h"

namespace gits::gui {

namespace Labels {
using ItemType = gits::ImGuiHelper::ItemType;
using SideBarItem = Context::SideBarItem;

// main window
static constexpr const char* OPEN_FOLDER_MENU = "Open Folder";
static constexpr const char* NEW_SESSION_BUTTON = "New Session";
static constexpr const char* NEW_SESSION_BUTTON_TOOLTIP =
    "Clear all paths except the GITS base path";

static constexpr const char* SUBCAPTURE = "Subcapture (DX)";
static constexpr const char* SUBCAPTURE_HINT = "Subcapture (DX only!) an existing stream";

static constexpr const char* SIMPLE_MODE_BACK_TO_SELECTION_BUTTON = "Back";
static constexpr const char* SIMPLE_MODE_LABEL = ICON_MDI_FLASH_OUTLINE " Simple Mode";
static constexpr const char* ADVANCED_MODE_LABEL = ICON_MDI_TOOLS " Advanced Mode";
static constexpr const char* GITS_BASE_FOLDER_MENU_ITEM = "GITS Base";
static constexpr const char* GITS_STREAM_PLAYBACK_FOLDER_MENU_ITEM = "Playback Stream";
static constexpr const char* GITS_STREAM_SUBCAPTURE_FOLDER_MENU_ITEM = "Subcapture Stream";
static constexpr const char* GITS_TARGET_FOLDER_MENU_ITEM = "Target";
static constexpr const char* GITS_CAPTURE_FOLDER_MENU_ITEM = "Capture";
static constexpr const char* GITS_ARTIFACTS_FOLDER_MENU_ITEM = "Artifacts";
static constexpr const char* GITS_TRACE_FOLDER_MENU_ITEM = "Trace";
static constexpr const char* GITS_SUBCAPTURE_FOLDER_MENU_ITEM = "Subcapture";
static constexpr const char* BASE_CONFIG_NOTICE =
    "This is a read-only view of the loaded config file";
static constexpr const char* DELTA_CONFIG_NOTICE = "Show only the difference to";
static constexpr const char* CUSTOM_ARGS_PLAYBACK_HINT =
    "Additional command line arguments for GITS Player. Not validated by GITS Launcher.";
static constexpr const char* CUSTOM_ARGS_PLAYBACK_HELP_TITLE = "GITS Player Custom Arguments";
static constexpr const char* CUSTOM_ARGS_PLAYBACK_HELP_HINT =
    "Set custom arguments to the GITS Player";
static constexpr const char* CUSTOM_ARGS_PLAYBACK_HELP_TEXT =
    "The Arguments set here are not validated by the GITS Launcher and have thus no influence on "
    "the YAML configuration shown. Config values set here have precedence over the YAML config "
    "file.";

// Capture-Options
static constexpr const char* RECORDING_ENABLED = "Enable recording";
static constexpr const char* SHADOW_MEMORY = "Enable shadow memory";
static constexpr const char* BASE_ON_TARGET_PATH = "Base on target path";

// Playback-Options

static constexpr const char* ARTIFACTS_LABEL = "Artifacts";
static constexpr const char* ARTIFACTS_PATH = "Path";
static constexpr const char* ARTIFACTS_HINT =
    "All artifacts (screenshots, resources, CCode) are written into respective subfolders";
static constexpr const char* ARTIFACTS_HELP_TITLE = "Artifacts";
static constexpr const char* ARTIFACTS_HELP_TOOLTIP =
    "Artifacts are screenshots, resources and CCode.";
static constexpr const char* ARTIFACTS_HELP_DESCRIPTION =
    "All artifacts (screenshots, resources, CCode) are written into respective subfolders of the "
    "specified path. If the path is empty, the artifacts will be written into the same folder as "
    "the stream file.";

//Subcapture
static constexpr const char* COMMAND_LIST_SUBCAPTURE = "Command List Subcapture";
static constexpr const char* COMMAND_LIST_SUBCAPTURE_NOTICE =
    "Creating a Command List Subcapture requires a serialized Subcapture.\n"
    "Please use the \"Serialize CPU and GPU execution\" option"
    " to create a serialized Subcapture, and then create a "
    "trace from it to select the execution indices";
static constexpr const char* COMMAND_LIST_SUBCAPTURE_FRAME = "Frame###CommandListSubcaptureFrame";
static constexpr const char* COMMAND_LIST_SUBCAPTURE_FRAME_HINT =
    "Frame for the command list subcapture";
static constexpr const char* COMMAND_LIST_SUBCAPTURE_FRAME_HELP =
    "Command list subcaptures can only be performed from "
    "a single frame";
static constexpr const char* COMMAND_LIST_SUBCAPTURE_EXECUTIONS_START =
    "Executions start###CommandListExecutionsStart";
static constexpr const char* COMMAND_LIST_SUBCAPTURE_EXECUTIONS_END =
    "Executions end###CommandListExecutionsEnd";

// Capture cleanup
static constexpr const char* GITS_ARTIFACTS_BUTTON = "Open Artifacts folder";
static constexpr const char* GITS_BASE_PATHS_MENU = "GITS base path";

#ifdef WIN32
// System setup panel
static constexpr const char* UNKNOWN_WINDOWS_ERROR = "Unknown error";
static constexpr const char* SYSTEM_SETUP_REGISTRY_KEYS_SEPARATOR = "Registry keys";
static constexpr const char* SYSTEM_SETUP_SHADER_CACHE_SEPARATOR = "Shader cache";
static constexpr const char* RESYNC_REGISTRY_BUTTON = "Resync Registry";
static constexpr const char* CLEAR_ALL_REGISTRY_KEYS_BUTTON = "Clear All Registry Keys";
static constexpr const char* REGISTRY_ACTIVE_COUNT_FORMAT = "(%d / %d active)";
static constexpr const char* SYSTEM_SETUP_ADMIN_REQUIRED_TEXT =
    "Registry operations require administrator privileges.";
static constexpr const char* SYSTEM_SETUP_RELAUNCH_AS_ADMIN_BUTTON = "Relaunch As Admin";
static constexpr const char* CLEAR_SHADER_CACHE_BUTTON = "Clear Shader Cache";
static constexpr const char* CLEAR_SHADER_CACHE_CONFIRM_POPUP_ID = "Clear Shader Cache##confirm";
static constexpr const char* CLEAR_SHADER_CACHE_CONFIRM_TEXT =
    "This will clear all files in the following directory:\n%s\n\nAre you sure you want to "
    "proceed?";
static constexpr const char* CLEAR_SHADER_CACHE_CONFIRM_YES_BUTTON = "Yes##clear_confirm";
static constexpr const char* CLEAR_SHADER_CACHE_CONFIRM_NO_BUTTON = "No##clear_cancel";
static constexpr const char* SHADER_CACHE_PATH_INTEL_DIR = "Intel";
static constexpr const char* SHADER_CACHE_PATH_CACHE_DIR = "ShaderCache";
static constexpr const char* REGISTRY_TOOLTIP_KEY_PATH_FORMAT = "%s";
static constexpr const char* REGISTRY_TOOLTIP_VALUE_FORMAT = "   -> %s = %s";
static constexpr const char* LOG_LOCAL_APPDATA_LOW_FAILURE =
    "[SystemSetupPanel] Failed to get LocalAppDataLow folder path\n";
static constexpr const char* LOG_SHADER_CACHE_CLEARED_PREFIX =
    "[SystemSetupPanel] Cleared shader cache at ";
static constexpr const char* LOG_SHADER_CACHE_REMOVED_PREFIX = " (removed ";
static constexpr const char* LOG_SHADER_CACHE_REMOVED_SUFFIX = " items)\n";
static constexpr const char* LOG_SHADER_CACHE_PATH_MISSING_PREFIX =
    "[SystemSetupPanel] Shader cache path does not exist: ";
static constexpr const char* LOG_SHADER_CACHE_PATH_MISSING_SUFFIX = "\n";
static constexpr const char* LOG_SHADER_CACHE_CLEAR_FAILURE_PREFIX =
    "[SystemSetupPanel] Failed to clear shader cache: ";
static constexpr const char* LOG_SHADER_CACHE_CLEAR_FAILURE_SUFFIX = "\n";
static constexpr const char* LOG_SHADER_CACHE_PARTIAL_PREFIX =
    "[SystemSetupPanel] Shader cache clear completed with skipped items at ";
static constexpr const char* LOG_SHADER_CACHE_PARTIAL_MIDDLE = " (removed ";
static constexpr const char* LOG_SHADER_CACHE_PARTIAL_SKIPPED_MIDDLE = " items, skipped ";
static constexpr const char* LOG_SHADER_CACHE_PARTIAL_SUFFIX = " items)\n";
static constexpr const char* LOG_SHADER_CACHE_ITERATION_ERROR_PREFIX =
    "[SystemSetupPanel] Failed while enumerating shader cache entries: ";
static constexpr const char* LOG_SHADER_CACHE_ITERATION_ERROR_SUFFIX = "\n";
static constexpr const char* LOG_LAUNCHER_ADMIN_MODE_PREFIX =
    "Launcher running with admin privileges: ";
static constexpr const char* LOG_LAUNCHER_ADMIN_MODE_YES = "yes";
static constexpr const char* LOG_LAUNCHER_ADMIN_MODE_NO = "no";
#endif

static constexpr const char* CAPTURE_MONITORING_TITLE = "Capture monitoring";
static constexpr const char* CAPTURE_MONITORING_STATE_ACTIVE = "Capture in progress";
static constexpr const char* CAPTURE_MONITORING_STATE_WAITING =
    "Waiting for 3-second quiet period after process exit";
static constexpr const char* CAPTURE_MONITORING_TIMER_FORMAT = " (%.1fs / %.0fs)";
static constexpr const char* CAPTURE_MONITORING_PID_LABEL = "PID";
static constexpr const char* CAPTURE_MONITORING_EXECUTABLE_LABEL = "Executable";
static constexpr const char* SUBCAPTURE_MONITORING_TITLE = "Subcapture";

static constexpr const char* OPTIONS_PANEL_OPTIONS = ICON_MDI_TUNE " Options";
static constexpr const char* OPTIONS_PANEL_PLUGINS = ICON_MDI_TOY_BRICK_OUTLINE " Plugins";
static constexpr const char* OPTIONS_PANEL_SYSTEM_SETUP = ICON_MDI_COG_OUTLINE " System setup";

static const auto& SIDE_BAR() {
  static const std::map<SideBarItem, ImGuiHelper::ButtonGroupItem> items = {
      {SideBarItem::LABEL_GITS,
       {.label = "GITS", .tooltip = "Settings for GITS", .type = ItemType::Label}},
      {SideBarItem::OPTIONS,
       {.label = "Configuration", .tooltip = "Show common configuration options"}},
      {SideBarItem::YAML_CONFIG, {.label = "YAML Config", .tooltip = "Textual yaml config files"}},
      {SideBarItem::LABEL_STREAM,
       {.label = "Stream", .tooltip = "Work with the stream", .type = ItemType::Label}},
      {SideBarItem::INFO,
       {.label = "Info", .tooltip = "Show stream information", .enabled = false}},
      {SideBarItem::STATS, {"Metadata", "Show the stream metadata"}},
      {SideBarItem::API_TRACE,
       {.label = "API-Trace", .tooltip = "Show an API-Trace of the stream", .enabled = false}},
      {SideBarItem::RESOURCE_DUMP,
       {.label = "Resource Dump", .tooltip = "Show the resource dump config"}},
      {SideBarItem::LABEL_LOG, {.label = "Log", .tooltip = "Show logs", .type = ItemType::Label}},
      {SideBarItem::LOG, {"GITS Log", "Show the GITS output"}},
      {SideBarItem::APP_LOG, {"Launcher Log", "Show the launcher log"}},
  };
  return items;
}

static const auto& SIDE_BAR_VEC() {
  static const std::vector<std::string> values = extractLabels(SIDE_BAR());
  return values;
}

static const auto& MODE_BUTTONS() {
  static const std::map<GitsAction, gits::ImGuiHelper::ButtonGroupItem> items = {
      {GitsAction::PLAYBACK, {ICON_MDI_MOVIE_PLAY " Playback", "Playback a GITS stream"}},
      {GitsAction::CAPTURE, {ICON_MDI_RECORD_REC " Capture", "Capture a GITS stream"}},
      {GitsAction::SUBCAPTURE, {ICON_MDI_RECORD_CIRCLE " Subcapture", "Subcapture a GITS stream"}},
  };
  return items;
}

static const auto& MODE_BUTTONS_VEC() {
  static const std::vector<std::string> values = extractLabels(MODE_BUTTONS());
  return values;
}

}; // namespace Labels
} // namespace gits::gui
