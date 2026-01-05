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
#include "context.h"

namespace gits::gui {

struct Labels {
  using SideBarItems = gits::gui::Context::SideBarItems;
  using ConfigSectionItems = gits::gui::Context::ConfigSectionItems;

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
  static constexpr const char* CHOOSE_STREAM = "Browse###STREAM";
  static constexpr const char* CHOOSE_STREAM_HINT =
      "Open File Dialog to choose a stream file (inside a folder)";
  static constexpr const char* CHOOSE_TARGET = "Browse###Target";
  static constexpr const char* CHOOSE_TARGET_HINT =
      "Open File Dialog to choose the target application";
  static constexpr const char* CHOOSE_GITSPLAYER = "Browse###GITS_PLAYER";
  static constexpr const char* CHOOSE_GITSPLAYER_HINT =
      "Open File Dialog to choose the GITS-Player to be used";
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
  static constexpr const char* SCREENSHOTS_PATH = "Screenshots Path";
  static constexpr const char* SCREENSHOTS_RANGES = "Screenshot Range(s)";
  static constexpr const char* SCREENSHOTS_START_FRAME = "Start Frame###Screenshot1";
  static constexpr const char* SCREENSHOTS_END_FRAME = "End Frame###Screenshot2";
  static constexpr const char* SCREENSHOTS_ADD_RANGE = "Add Range";
  static constexpr const char* SCREENSHOTS_ADD_FRAME = "Add Start Frame";
  static constexpr const char* TRACE_EXPORT = "Export Trace";
  static constexpr const char* TRACE_PATH = "Trace Path";
  static constexpr const char* SUBCAPTURE_RANGE = "Subcapture Range";
  static constexpr const char* SUBCAPTURE_START_FRAME = "Start Frame###Subcapture3";
  static constexpr const char* SUBCAPTURE_END_FRAME = "End Frame###Subcapture4";
  static constexpr const char* SUBCAPTURE_STEP_FRAME = "Step Size###Subcapture5";

  //Subcapture
  static constexpr const char* SUBCAPTURE_PATH = "Subcapture Path";
  static constexpr const char* SUBCAPTURE_OPTIMIZE = "Optimize Subcapture";
  static constexpr const char* SUBCAPTURE_OPTIMIZE_RAY = "Optimize Subcapture Raytracing";

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
  static constexpr const char* GITS_STREAM_BUTTON = "Open Stream folder";
  static constexpr const char* GITS_TARGET_BUTTON = "Open Target folder";
  static constexpr const char* GITS_SCREENSHOT_BUTTON = "Open Screenshots folder";
  static constexpr const char* GITS_TRACE_BUTTON = "Open Trace folder";
  static constexpr const char* GITS_SUBCAPTURE_BUTTON = "Open Subcapture folder";

  static const std::string MainAction(gits::gui::Context::MainAction action) {
    switch (action) {
    case gits::gui::Context::MainAction::PLAYBACK:
      return "Start Playback";
    case gits::gui::Context::MainAction::CAPTURE:
      return "Start Capturing";
    case gits::gui::Context::MainAction::STATISTICS:
      return "Gather Statistics";
    case gits::gui::Context::MainAction::SUBCAPTURE:
      return "Subcapture Stream";
    case gits::gui::Context::MainAction::COUNT:
    default:
      return "";
    }
  }

  static const auto& SIDE_BAR() {
    static const std::map<SideBarItems, ImGuiHelper::ButtonGroupItem> items = {
        {SideBarItems::CONFIG, {"Config YML", "Show current GITS config"}},
        {SideBarItems::OPTIONS, {.label = "Config - UI", .tooltip = "Show easy to setup options"}},
        {SideBarItems::CLI, {"CLI", "Show the full command line call"}},
        {SideBarItems::LOG, {"GITS Log", "Show the GITS output"}},
        {SideBarItems::STATS, {"Stats", "Show statistics of the stream"}},
        {SideBarItems::APP_LOG, {"Launcher Log", "Show the launcher log"}},
        {SideBarItems::INFO,
         {.label = "Info", .tooltip = "Show stream information", .enabled = false}},
        {SideBarItems::API_TRACE,
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

  static const auto& CONFIG_SECTIONS() {
    static const std::map<ConfigSectionItems, ImGuiHelper::ButtonGroupItem> labels = {
        {ConfigSectionItems::COMMON,
         {"Common", "Common options", ImGuiHelper::ButtonStatus::Default, "Cmn"}},
        {ConfigSectionItems::DIRECTX,
         {"DirectX", "DirectX API options", ImGuiHelper::ButtonStatus::Default, "DX"}},
        {ConfigSectionItems::OPENGL,
         {"OpenGL", "OpenGL API options", ImGuiHelper::ButtonStatus::Default, "GL"}},
        {ConfigSectionItems::VULKAN,
         {"Vulkan", "Vulkan API options", ImGuiHelper::ButtonStatus::Default, "VK"}},
        {ConfigSectionItems::OPENCL,
         {"OpenCL", "OpenCL API options", ImGuiHelper::ButtonStatus::Default, "CL"}},
        {ConfigSectionItems::LEVELZERO,
         {"LevelZero", "LevelZero API options", ImGuiHelper::ButtonStatus::Default, "L0"}},
        {ConfigSectionItems::OVERRIDES,
         {"Overrides", "Per application overrides", ImGuiHelper::ButtonStatus::Default, "Ovr"}},
    };
    return labels;
  }

  float static MaxLength(const std::vector<std::string>& labels) {
    float maximum = std::numeric_limits<float>::min();
    for (const auto& label : labels) {
      maximum = std::max(maximum, ImGuiHelper::WidthOf(ImGuiHelper::Widgets::Label, label));
    }
    return maximum;
  }
};
} // namespace gits::gui
