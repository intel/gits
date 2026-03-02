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
#include "common.h"
#include "context.h"

namespace gits::gui {

struct Labels {
  using SideBarItem = Context::SideBarItem;
  using ConfigSectionItem = Context::ConfigSectionItem;
  using MetaDataItem = Context::MetaDataItem;

  // main window
  static constexpr const char* VERSION = "Version";
  static constexpr const char* TITLE = "GITS Launcher";
  static constexpr const char* NOTICE = "GITS Launcher is currently designed to support DirectX. "
                                        "Not all features work for other APIs.";
  static constexpr const char* NOTICE_2 = "This feature is exclusive to DirectX.";
  static constexpr const char* CHOOSE_CONFIG = "Browse";
  static constexpr const char* CHOOSE_CONFIG_HINT =
      "Open File Dialog to choose a configuration yaml";
  static constexpr const char* CHOOSE_CAPTURE_CONFIG = "Browse";
  static constexpr const char* CHOOSE_CAPTURE_CONFIG_HINT =
      "Open File Dialog to choose a configuration yaml";
  static constexpr const char* STREAM = "Stream";
  static constexpr const char* STREAM_HINT = "Select a stream";
  static constexpr const char* TARGET = "Target";
  static constexpr const char* TARGET_HINT = "Select an application to run the stream with";
  static constexpr const char* CAPTURE = "Capture";
  static constexpr const char* CAPTURE_HINT = "Capture a new stream";
  static constexpr const char* PLAYBACK = "Playback";
  static constexpr const char* PLAYBACK_HINT = "Playback an existing stream";
  static constexpr const char* SUBCAPTURE = "Subcapture (DX)";
  static constexpr const char* SUBCAPTURE_HINT = "Subcapture (DX only!) an existing stream";
  static constexpr const char* START = "Start";
  static constexpr const char* START_HINT = "Start playback/capture";
  static constexpr const char* START_CAPTURE = "Start";
  static constexpr const char* START_CAPTURE_HINT = "Start capture";
  static constexpr const char* PLAYER_PATH = "GITS-Player";
  static constexpr const char* PLAYER_PATH_HINT = "Path to GITS-Player";
  static constexpr const char* BASE_PATH = "GITS base path";
  static constexpr const char* BASE_PATH_HINT = "Path to GITS base path";
  static constexpr const char* CONFIG = "Config";
  static constexpr const char* CONFIG_HINT = "Path to GITS config file";
  static constexpr const char* CAPTURE_CONFIG = "Config";
  static constexpr const char* CAPTURE_CONFIG_HINT = "Path to GITS config file";
  static constexpr const char* CLEANUP = "Cleanup";
  static constexpr const char* CHOOSE_STREAM = "Browse###STREAM";
  static constexpr const char* CHOOSE_STREAM_HINT =
      "Open File Dialog to choose a stream file (inside a folder)";
  static constexpr const char* CHOOSE_TARGET = "Browse###Target";
  static constexpr const char* CHOOSE_TARGET_HINT =
      "Open File Dialog to choose the target application";
  static constexpr const char* CHOOSE_GITSPLAYER = "Browse###GITS_PLAYER";
  static constexpr const char* CHOOSE_GITSPLAYER_HINT =
      "Open File Dialog to choose the GITS-Player to be used";
  static constexpr const char* USE_CUSTOM_GITSPLAYER = "Custom GITS Player";
  static constexpr const char* GITSPLAYER_TOOLTIP =
      "Enable to specify a custom GITS-Player executable path";
  static constexpr const char* CHOOSE_GITS_BASE_PATH = "Browse###GITS_BASE";
  static constexpr const char* CHOOSE_GITS_BASE_PATH_HINT =
      "Open File Dialog to choose the GITS base path to be used";
  static constexpr const char* CUSTOM_ARGS = "Custom Args";
  static constexpr const char* CUSTOM_ARGS_INPUT_HINT =
      "Additional command line arguments for the target application";
  static constexpr const char* CAPTURE_CUSTOM_ARGS = "Custom Args";
  static constexpr const char* CAPTURE_CUSTOM_ARGS_INPUT_HINT =
      "Additional command line arguments for the target application";
  static constexpr const char* CLEAR_ARGUMENTS = "Clear";
  static constexpr const char* CLEAR_ARGUMENTS_HINT = "Clear all custom arguments in the textbox";
  static constexpr const char* CLEAR_CAPTURE_ARGUMENTS = "Clear";
  static constexpr const char* CLEAR_CAPTURE_ARGUMENTS_HINT =
      "Clear all custom arguments in the textbox";
  static constexpr const char* CAPTURE_OUTPUT_PATH = "Output path";
  static constexpr const char* CAPTURE_OUTPUT_PATH_HINT =
      "Path where the resulting stream file will be outputted to";
  static constexpr const char* CHOOSE_CAPTURE_OUTPUT_PATH = "Browse###CAPTURE_OUTPUT";
  static constexpr const char* CHOOSE_CAPTURE_OUTPUT_PATH_HINT =
      "Open File Dialog to choose the stream output path to be used";
  static constexpr const char* OPEN_CAPTURE_OUTPUT_PATH = "Open";
  static constexpr const char* OPEN_CAPTURE_OUTPUT_PATH_HINT = "Open specified path in explorer";

  // EZ-Options
  static constexpr const char* HUD_ENABLED = "Show HUD";
  static constexpr const char* SCREENSHOTS = "Screenshots";
  static constexpr const char* SCREENSHOTS_PATH = "Path";
  static constexpr const char* SCREENSHOTS_RANGES = "Range";
  static constexpr const char* SCREENSHOTS_START_FRAME = "Start Frame###Screenshot1";
  static constexpr const char* SCREENSHOTS_END_FRAME = "End Frame###Screenshot2";
  static constexpr const char* SCREENSHOTS_ADD_RANGE = "Add Range";
  static constexpr const char* SCREENSHOTS_ADD_FRAME = "Add Start Frame";
  static constexpr const char* TRACE_EXPORT = "Export Trace";
  static constexpr const char* TRACE_PATH = "Trace Path";

  //Subcapture
  static constexpr const char* SUBCAPTURE_RANGE = "Subcapture Range";
  static constexpr const char* SUBCAPTURE_START_FRAME = "Start Frame###Subcapture3";
  static constexpr const char* SUBCAPTURE_END_FRAME = "End Frame###Subcapture4";
  static constexpr const char* SUBCAPTURE_STEP_FRAME = "Step Size###Subcapture5";
  static constexpr const char* SUBCAPTURE_PATH = "Subcapture Path";
  static constexpr const char* SUBCAPTURE_OPTIMIZE = "Optimize Subcapture";
  static constexpr const char* SUBCAPTURE_EXECUTION_SERIALIZATION =
      "Serialize CPU and GPU execution";
  static constexpr const char* SUBCAPTURE_COMMAND_LIST_EXECUTIONS = "Command lists execution range";

  // Capture cleanup
  static constexpr const char* CLEAN_RECORDER_FILES = "Recorder files";
  static constexpr const char* CLEAN_RECORDER_FILES_HINT =
      "Remove the recorder files (e.g. DLLs) after recording";
  static constexpr const char* CLEAN_RECORDER_CONFIG = "Config";
  static constexpr const char* CLEAN_RECORDER_CONFIG_HINT =
      "Remove the recorder config file after recording";
  static constexpr const char* CLEAN_RECORDER_LOG = "Log";
  static constexpr const char* CLEAN_RECORDER_LOG_HINT =
      "Remove the recorder log file after recording";
  static constexpr const char* FORCE_CLEANUP = "Force cleanup";
  static constexpr const char* FORCE_CLEANUP_HINT = "Force cleanup of the selected files";

  // main window - text fields
  static constexpr const char* STREAM_INPUT_HINT = "Path to stream";
  static constexpr const char* TARGET_INPUT_HINT = "Path to target executable";
  static constexpr const char* PLAYER_PATH_INPUT_HINT = "Path to GITS Player executable";
  static constexpr const char* BASE_PATH_INPUT_HINT = "Path to GITS base directory";
  static constexpr const char* CONFIG_INPUT_HINT = "Path to GITS config file";
  static constexpr const char* CAPTURE_CONFIG_INPUT_HINT = "Path to GITS config file";
  static constexpr const char* CAPTURE_OUTPUT_PATH_INPUT_HINT =
      "Path where to output the stream to";

  static constexpr const char* GITS_BASE_BUTTON = "Open GITS Base folder";
  static constexpr const char* GITS_STREAM_PLAYBACK_BUTTON = "Open Playback Stream folder";
  static constexpr const char* GITS_STREAM_SUBCAPTURE_BUTTON = "Open Subcapture Stream folder";
  static constexpr const char* GITS_TARGET_BUTTON = "Open Target folder";
  static constexpr const char* GITS_CAPTURE_BUTTON = "Open Capture folder";
  static constexpr const char* GITS_SCREENSHOT_BUTTON = "Open Screenshots folder";
  static constexpr const char* GITS_TRACE_BUTTON = "Open Trace folder";
  static constexpr const char* GITS_SUBCAPTURE_BUTTON = "Open Subcapture folder";
  static constexpr const char* RESET_BASE_PATHS = "Reset GITS base path";

  static constexpr const char* API_LABEL = "API";
  static constexpr const char* API_NAME_DX = "DirectX";
  static constexpr const char* API_NAME_SHORT_DX = "DX";
  static constexpr const char* API_NAME_GL = "OpenGL";
  static constexpr const char* API_NAME_SHORT_GL = "GL";
  static constexpr const char* API_NAME_VK = "Vulkan";
  static constexpr const char* API_NAME_SHORT_VK = "VK";
  static constexpr const char* API_NAME_CL = "OpenCL";
  static constexpr const char* API_NAME_SHORT_CL = "CL";
  static constexpr const char* API_NAME_L0 = "LevelZero";
  static constexpr const char* API_NAME_SHORT_L0 = "L0";

  static constexpr const char* NOT_AVAILABLE = "N/A";

  // MetaData
  static constexpr const char* NULLOPT_STREAM_PATH_MESSAGE = "Couldn't retrieve stream path";
  static constexpr const char* EMPTY_STREAM_PATH_MESSAGE =
      "Can't get stream metadata, no stream path was selected";
  static constexpr const char* STREAM_PATH_DOESNT_EXIST_MESSAGE =
      "Can't get stream metadata, selected stream path doesn't exist";
  static constexpr const char* STATS_GATHERING_IN_PROGRESS_MESSAGE = "Gathering stream stats...";
  static constexpr const char* STATS_GATHERING_PROMPT_MESSAGE =
      "Click the button below to gather stream stats (this launches the gits player).";
  static constexpr const char* STATS_GATHERING_BUTTON_LABEL = "Gather stats";
  static constexpr const char* UNKNOWN_METADATA_TAB_MESSAGE = "Couldn't get stream meta data";
  static constexpr const char* EMPTY_RECORDER_DIAGS_MESSAGE =
      "Couldn't get recorder diagnostic information for given trace.";
  static constexpr const char* EMPTY_RECORDER_CONFIG_MESSAGE =
      "Couldn't get recorder config for given trace.";

  static const std::string MainAction(Mode action) {
    switch (action) {
    case Mode::PLAYBACK:
      return "Start Playback";
    case Mode::CAPTURE:
      return "Start Capture";
    case Mode::SUBCAPTURE:
      return "Start Subcapture";
    case Mode::COUNT:
    default:
      return "";
    }
  }

  static const std::string DialogTitle(FileDialogKeys key) {
    switch (key.Path) {
    case Path::GITS_BASE:
      return "Choose GITS installation directory";
      break;
    case Path::CUSTOM_PLAYER:
      return "Choose custom GITS player";
      break;
    case Path::SCREENSHOTS:
      return "Choose directory where to output screenshots";
      break;
    case Path::TRACE:
      return "Choose directory where to output trace info";
      break;
    case Path::CAPTURE_TARGET:
      return "Choose target application";
      break;
    case Path::CONFIG:
      return "Choose GITS config";
      break;
    case Path::INPUT_STREAM:
      return "Choose gits stream";
      break;
    case Path::OUTPUT_STREAM:
      return "Choose directory where to output the stream to";
    default:
      return "";
    };
  }

  static const std::string PlaceholderText(Mode mode) {
    switch (mode) {
    case Mode::PLAYBACK:
      return "PLAYBACK IN PROGRESS";
    case Mode::CAPTURE:
      return "CAPTURE IN PROGRESS";
    case Mode::SUBCAPTURE:
      return "SUBCAPTURE IN PROGRESS";
    case Mode::COUNT:
    default:
      return "";
    };
  }

  static const auto& SIDE_BAR() {
    static const std::map<SideBarItem, ImGuiHelper::ButtonGroupItem> items = {
        {SideBarItem::OPTIONS,
         {.label = "Configuration", .tooltip = "Show common configuration options"}},
        {SideBarItem::CONFIG, {"Config YML", "Show current GITS config file"}},
        {SideBarItem::CLI, {"CLI", "Show the full command line call"}},
        {SideBarItem::LOG, {"GITS Log", "Show the GITS output"}},
        {SideBarItem::STATS, {"Metadata", "Show the stream metadata"}},
        {SideBarItem::APP_LOG, {"Launcher Log", "Show the launcher log"}},
        {SideBarItem::INFO,
         {.label = "Info", .tooltip = "Show stream information", .enabled = false}},
        {SideBarItem::API_TRACE,
         {.label = "API-Trace", .tooltip = "Show an API-Trace of the stream", .enabled = false}},
    };
    return items;
  }
  static const auto& SIDE_BAR_VEC() {
    static const std::vector<std::string> values([] {
      std::vector<std::string> result;
      for (const auto& pair : SIDE_BAR()) {
        result.push_back(pair.second.label);
      }
      return result;
    }());
    return values;
  }

  static const auto& MODE_BUTTONS() {
    static const std::map<Mode, gits::ImGuiHelper::ButtonGroupItem> items = {
        {Mode::PLAYBACK, {"Playback", "Playback a gits stream"}},
        {Mode::CAPTURE, {"Capture", "Capture a gits stream"}},
        {Mode::SUBCAPTURE, {"Subcapture", "Subcapture a gits stream"}},
    };
    return items;
  }

  static const auto& MODE_BUTTONS_VEC() {
    static const std::vector<std::string> values([] {
      std::vector<std::string> result;
      for (const auto& pair : MODE_BUTTONS()) {
        result.push_back(pair.second.label);
      }
      return result;
    }());
    return values;
  }

  static const auto& CONFIG_SECTIONS() {
    static const std::map<ConfigSectionItem, ImGuiHelper::ButtonGroupItem> labels = {
        {ConfigSectionItem::COMMON,
         {"Common", "Common options", ImGuiHelper::ButtonStatus::Default, "Cmn"}},
        {ConfigSectionItem::DIRECTX,
         {API_NAME_DX, "DirectX API options", ImGuiHelper::ButtonStatus::Default,
          API_NAME_SHORT_DX}},
        {ConfigSectionItem::OPENGL,
         {API_NAME_GL, "OpenGL API options", ImGuiHelper::ButtonStatus::Default,
          API_NAME_SHORT_GL}},
        {ConfigSectionItem::VULKAN,
         {API_NAME_VK, "Vulkan API options", ImGuiHelper::ButtonStatus::Default,
          API_NAME_SHORT_VK}},
        {ConfigSectionItem::OPENCL,
         {API_NAME_CL, "OpenCL API options", ImGuiHelper::ButtonStatus::Default,
          API_NAME_SHORT_CL}},
        {ConfigSectionItem::LEVELZERO,
         {API_NAME_L0, "LevelZero API options", ImGuiHelper::ButtonStatus::Default,
          API_NAME_SHORT_L0}},
        {ConfigSectionItem::OVERRIDES,
         {"Overrides", "Per application overrides", ImGuiHelper::ButtonStatus::Default, "Ovr"}},
    };
    return labels;
  }

  static const auto& META_DATA() {
    static const std::map<MetaDataItem, ImGuiHelper::ButtonGroupItem> labels = {
        {MetaDataItem::CONFIG,
         {"Config", "Config the stream was recorded with", ImGuiHelper::ButtonStatus::Default,
          "Config"}},
        {MetaDataItem::STATS,
         {"Stats", "Stream statistics", ImGuiHelper::ButtonStatus::Default, "Stats"}},
        {MetaDataItem::DIAGS,
         {"Diagnostics", "Stream Recorder diagnostics", ImGuiHelper::ButtonStatus::Default,
          "Diags"}},
    };
    return labels;
  };

  float static MaxLength(const std::vector<std::string>& labels) {
    float maximum = std::numeric_limits<float>::min();
    for (const auto& label : labels) {
      maximum = std::max(maximum, ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Label, label));
    }
    return maximum;
  }
};
} // namespace gits::gui
