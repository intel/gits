// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "launcherActions.h"

#include "ImGuiFileDialog.h"

#include "fileActions.h"
#include "labels.h"
#include "captureActions.h"
#include "log.h"

namespace gits::gui {

typedef gits::gui::Context::SideBarItems SideBarItems;

const std::string str(FileDialogKeys key) {
  switch (key) {
  case FileDialogKeys::PICK_STREAM_PATH:
    return "PickStreamPath";
  case FileDialogKeys::PICK_TARGET_PATH:
    return "PickTargetPath";
  case FileDialogKeys::PICK_GITSPLAYER_PATH:
    return "PickGitsPlayerPath";
  case FileDialogKeys::PICK_GITS_BASE_PATH:
    return "PickGitsBasePath";
  case FileDialogKeys::PICK_CONFIG_PATH:
    return "PickConfigPath";
  case FileDialogKeys::PICK_CAPTURE_CONFIG_PATH:
    return "PickCaptureConfigPath";
  case FileDialogKeys::PICK_CAPTURE_OUTPUT_PATH:
    return "PickCaptureOutputPath";
  case FileDialogKeys::PICK_SUBCAPTURE_PATH:
    return "PickSubcapturePath";
  case FileDialogKeys::PICK_TRACE_PATH:
    return "PickTracePath";
  case FileDialogKeys::PICK_SCREENSHOTS_PATH:
    return "PickScreenshotsPath";
  default:
    return "Unknown";
  }
}

bool ValidateGITSConfigFile(std::filesystem::path configPath) {
  if (!std::filesystem::exists(configPath)) {
    LOG_ERROR << "Configuration file does not exist: " << configPath;
    return false;
  }

  //// In the future a more detailed validation can be added here
  //const auto result = gits::Configurator::Instance().Load(configPath);
  //if (!result) {
  //  LOG_ERROR << "Error reading in configuration from: " << configPath;
  //  return false;
  //}

  return true;
}

void UpdateCLICall(gui::Context& context) {
  context.UpdateFixedLauncherArguments();

  switch (context.AppMode) {
  case Context::Mode::PLAYBACK:
  case Context::Mode::SUBCAPTURE: {

    const auto gitsExecutable = context.GetPath(gui::Context::Paths::GITS_PLAYER);

    context.CLIArguments.clear();

    context.CLIArguments.push_back("--config=" +
                                   context.GetPath(gui::Context::Paths::CONFIG).string());
    context.CLIArguments.push_back(context.FixedLauncherArguments);
    context.CLIArguments.push_back(context.CustomArguments);
    context.CLIArguments.push_back(context.GetPath(gui::Context::Paths::STREAM).string());

    context.CLIArguments.erase(std::remove_if(context.CLIArguments.begin(),
                                              context.CLIArguments.end(),
                                              [](const std::string& arg) { return arg.empty(); }),
                               context.CLIArguments.end());

    std::string cliEditorText =
        "# This buffer is write protected, it shows the current command line\n\n";

    cliEditorText += gitsExecutable.string() + "\n";
    if (!std::filesystem::exists(gitsExecutable)) {
      cliEditorText += "!!  [Warning: GITS-Player path does not exist]\n";
    }

    for (const auto& argument : context.CLIArguments) {
      cliEditorText += "  " + argument + "\n";
    }
    context.CLIEditor->SetText(cliEditorText);
    break;
  }
  case Context::Mode::CAPTURE: {

    const auto targetExecutable = context.GetPath(gui::Context::Paths::TARGET);

    context.CLIArguments.clear();
    context.CLIArguments.push_back(context.CaptureCustomArguments);

    context.CLIArguments.erase(std::remove_if(context.CLIArguments.begin(),
                                              context.CLIArguments.end(),
                                              [](const std::string& arg) { return arg.empty(); }),
                               context.CLIArguments.end());

    std::string cliEditorText =
        "# This buffer is write protected, it shows the current command line\n\n";

    cliEditorText += targetExecutable.string() + "\n";
    if (!std::filesystem::exists(targetExecutable)) {
      cliEditorText += "!!  [Warning: Target executable path does not exist]\n";
    }

    for (const auto& argument : context.CLIArguments) {
      cliEditorText += "  " + argument + "\n";
    }
    context.CLIEditor->SetText(cliEditorText);
    break;
  }
  default:
    break;
  }
}

/*
void PlaybackStream(gui::Context& context) {
  const auto gitsPlayerPath = context.GetPath(gui::Context::Paths::GITS_PLAYER);

  context.GITSLogEditor->SetText("");
  FileActions::LaunchExecutableAsync(
      gitsPlayerPath, context.CLIArguments, gitsPlayerPath.parent_path(),
      std::bind(&gui::Context::GITSLog, context, std::placeholders::_1));
  context.BtnsSideBar->SelectEntry(SideBarItems::LOG);
}

void GetTraceStats(gui::Context& context) {
  const auto gitsPlayerPath = context.GetPath(gui::Context::Paths::GITS_PLAYER);

  context.GITSLogEditor->SetText("");
  FileActions::LaunchExecutableAsync(
      gitsPlayerPath, {"--stats", context.GetPath(gui::Context::Paths::STREAM).string()},
      gitsPlayerPath.parent_path(),
      std::bind(&gui::Context::TraceStats, context, std::placeholders::_1));
  context.BtnsSideBar->SelectEntry(SideBarItems::STATS);
}
*/
void PlaybackStream(gui::Context& context) {
  const auto gitsPlayerPath = context.GetPath(gui::Context::Paths::GITS_PLAYER);

  context.GITSLogEditor->SetText("");
  FileActions::LaunchExecutableAsync(gitsPlayerPath, context.CLIArguments,
                                     gitsPlayerPath.parent_path(),
                                     [&context](const std::string& log) { context.GITSLog(log); });
  context.BtnsSideBar->SelectEntry(SideBarItems::LOG);
}

void SubcaptureStream(gui::Context& context) {
  const auto gitsPlayerPath = context.GetPath(gui::Context::Paths::GITS_PLAYER);

  context.GITSLogEditor->SetText("");
  // TODO: Create a generic solution for this
  std::thread([gitsPlayerPath, &context]() {
    context.SubcaptureInProgress = true;
    // since we need to run player twice, consecutively & blocking, we do it in a separate thread
    FileActions::LaunchExecutable(gitsPlayerPath, context.CLIArguments, true,
                                  gitsPlayerPath.parent_path(),
                                  [&context](const std::string& log) { context.GITSLog(log); });
    FileActions::LaunchExecutable(gitsPlayerPath, context.CLIArguments, true,
                                  gitsPlayerPath.parent_path(),
                                  [&context](const std::string& log) { context.GITSLog(log); });
    context.SubcaptureInProgress = false;
  }).detach();

  context.BtnsSideBar->SelectEntry(SideBarItems::LOG);
}

void GetTraceStats(gui::Context& context) {
  const auto gitsPlayerPath = context.GetPath(gui::Context::Paths::GITS_PLAYER);

  context.GITSLogEditor->SetText("");
  FileActions::LaunchExecutableAsync(
      gitsPlayerPath, {"--stats", context.GetPath(gui::Context::Paths::STREAM).string()},
      gitsPlayerPath.parent_path(),
      [&context](const std::string& stats) { context.TraceStats(stats); });
  context.BtnsSideBar->SelectEntry(SideBarItems::STATS);
}
void UpdateConfigSectionPositions(gui::Context* context, const std::vector<std::string>& config) {
  size_t idx = 0;
  for (const auto& item : Labels::CONFIG_SECTIONS()) {
    int pos = -1;
    const std::string sectionHeader = item.second.label + ":";
    for (size_t lineNum = 0; lineNum < config.size(); ++lineNum) {
      const std::string& line = config[lineNum];
      if (line.compare(0, sectionHeader.size(), sectionHeader) == 0) {
        pos = static_cast<int>(lineNum);
        break;
      }
    }
    context->ConfigSectionLines[item.first] = pos;
    idx++;
  }
}

std::optional<std::string> ProcessFileDialog(FileDialogKeys key) {
  std::optional<std::string> result = std::nullopt;
  if (ImGuiFileDialog::Instance()->Display(str(key), 32, ImVec2(500, 300))) {
    if (ImGuiFileDialog::Instance()->IsOk()) {
      result = ImGuiFileDialog::Instance()->GetFilePathName();
    }
    ImGuiFileDialog::Instance()->Close();
  }
  return result;
}

void FileDialogs(gui::Context& context) {
  bool update = false;
  if (ProcessFileDialog(FileDialogKeys::PICK_STREAM_PATH)) {
    context.StreamPath = ImGuiFileDialog::Instance()->GetFilePathName();
    update = true;
  } else if (ProcessFileDialog(FileDialogKeys::PICK_TARGET_PATH)) {
    context.TargetPath = ImGuiFileDialog::Instance()->GetFilePathName();
    update = true;
  } else if (ProcessFileDialog(FileDialogKeys::PICK_CONFIG_PATH)) {
    context.ConfigPath = ImGuiFileDialog::Instance()->GetFilePathName();
    update = true;
    LoadConfigFile(&context);
  } else if (ProcessFileDialog(FileDialogKeys::PICK_CAPTURE_CONFIG_PATH)) {
    context.CaptureConfigPath = ImGuiFileDialog::Instance()->GetFilePathName();
    LoadConfigFile(&context);
  } else if (ProcessFileDialog(FileDialogKeys::PICK_GITSPLAYER_PATH)) {
    context.GITSPlayerPath = ImGuiFileDialog::Instance()->GetFilePathName();
    update = true;
  } else if (ProcessFileDialog(FileDialogKeys::PICK_GITS_BASE_PATH)) {
    context.GITSBasePath = ImGuiFileDialog::Instance()->GetCurrentPath();
  } else if (ProcessFileDialog(FileDialogKeys::PICK_CAPTURE_OUTPUT_PATH)) {
    context.CaptureOutputPath = ImGuiFileDialog::Instance()->GetCurrentPath();
    if (!gits::gui::capture_actions::UpdateConfigDumpPath(context)) {
      context.BtnsSideBar->SelectEntry(SideBarItems::APP_LOG);
    }
    LoadConfigFile(&context);
  } else if (ProcessFileDialog(FileDialogKeys::PICK_SUBCAPTURE_PATH)) {
    context.SubcapturePath = ImGuiFileDialog::Instance()->GetCurrentPath();
    update = true;
  } else if (ProcessFileDialog(FileDialogKeys::PICK_TRACE_PATH)) {
    context.TracePath = ImGuiFileDialog::Instance()->GetCurrentPath();
    update = true;
  } else if (ProcessFileDialog(FileDialogKeys::PICK_SCREENSHOTS_PATH)) {
    context.ScreenshotPath = ImGuiFileDialog::Instance()->GetCurrentPath();
    update = true;
  }

  if (update) {
    UpdateCLICall(context);
  }
}

void ShowFileDialog(gui::Context* context, FileDialogKeys key) {
  std::string title;
  std::string ext;
  IGFD::FileDialogConfig dlgConfig;
  dlgConfig.flags = ImGuiFileDialogFlags_Modal;

  switch (key) {
  case FileDialogKeys::PICK_STREAM_PATH:
    dlgConfig.filePathName = context->StreamPath.string();
    title = "Choose gits stream";
    ext = ".gits2";
    break;
  case FileDialogKeys::PICK_TARGET_PATH:
    dlgConfig.path = context->TargetPath.string();
    title = "Choose target application";
    ext = ".exe";
    break;
  case FileDialogKeys::PICK_GITSPLAYER_PATH:
    dlgConfig.filePathName = context->GITSPlayerPath.string();
    title = "Choose a GITS-Player";
    ext = ".exe";
    break;
  case FileDialogKeys::PICK_GITS_BASE_PATH:
    dlgConfig.path = context->GITSBasePath.string();
    title = "Choose GITS installation directory";
    break;
  case FileDialogKeys::PICK_CONFIG_PATH:
    dlgConfig.filePathName = context->ConfigPath.string();
    title = "Choose GITS config";
    ext = ".yml";
    break;
  case FileDialogKeys::PICK_CAPTURE_CONFIG_PATH:
    dlgConfig.filePathName = context->CaptureConfigPath.string();
    title = "Choose gits config for capture";
    ext = ".yml";
    break;
  case FileDialogKeys::PICK_CAPTURE_OUTPUT_PATH:
    dlgConfig.path = context->CaptureOutputPath.string();
    title = "Choose directory where to output the stream to";
    break;
  case FileDialogKeys::PICK_SUBCAPTURE_PATH:
    dlgConfig.path = context->SubcapturePath.string();
    title = "Choose gits subcapture stream path";
    break;
  case FileDialogKeys::PICK_TRACE_PATH:
    dlgConfig.path = context->TracePath.string();
    title = "Choose directory where to output trace info";
    break;
  case FileDialogKeys::PICK_SCREENSHOTS_PATH:
    dlgConfig.path = context->ScreenshotPath.string();
    title = "Choose directory where to output screenshots";
    break;
  default:
    LOG_ERROR << "Unknown/unimplemented file dialog key requested:" << int(key) << " - "
              << str(key);
    return;
  }
  ImGuiFileDialog::Instance()->OpenDialog(str(key), title, ext.empty() ? nullptr : ext.c_str(),
                                          dlgConfig);
}

void LoadConfigFile(gui::Context* context) {
  auto filePath = context->GetPath(context->IsPlayback() ? gui::Context::Paths::CONFIG
                                                         : gui::Context::Paths::CAPTURE_CONFIG);
  auto fhandle = std::ifstream(filePath);
  if (fhandle.is_open()) {
    const std::string str((std::istreambuf_iterator<char>(fhandle)),
                          std::istreambuf_iterator<char>());
    context->ConfigEditor->SetText(str);
    UpdateConfigSectionPositions(context, context->ConfigEditor->GetEditor().GetTextLines());
    TextEditor::Breakpoints breakpoints;
    for (const auto& section : context->ConfigSectionLines) {
      int line = section.second;
      context->BtnsAPI->SetEnabled(section.first, line >= 0);
      if (line >= 0) {
        breakpoints.insert(line + 1);
      }
    }
    context->ConfigEditor->SetFilePath(filePath);
    context->ConfigEditor->SetBreakpoints(breakpoints);
  } else {
    context->ConfigEditor->SetText("// Could not open file: " + filePath.string());
  }
}

void SetImGuiStyle(gui::Context* context, size_t idx) {
  context->LauncherConfiguration.Theme.SetThemeByIdx(idx);
  context->LauncherConfiguration.Theme.ApplyTheme();
  context->UpdatePalette();
}

void OpenURL(const std::string& url) {
#ifdef _WIN32
  system(("start " + url).c_str());
#elif __APPLE__
  system(("open " + url).c_str());
#elif __linux__
  system(("xdg-open " + url).c_str());
#endif
}

bool OpenFolder(const std::filesystem::path& path) {
  if (path.empty()) {
    LOG_ERROR << "Couldn't open directory. No path was provided.";

    return false;
  }

  if (!std::filesystem::exists(path)) {
    LOG_ERROR << "Given directory: " << path << " doesn't exist.";

    return false;
  }

  if (!std::filesystem::is_directory(path)) {
    LOG_ERROR << "Given path: " << path << " is not a directory";

    return false;
  }

#ifdef _WIN32
  int result = system(("explorer " + path.string()).c_str());
#elif __APPLE__
  int result = system(("open " + path.string()).c_str());
#elif __linux__
  int result = system(("xdg-open " + path.string()).c_str());
#endif

  return result == 0;
}

bool OpenFolder(const std::string& path) {
  return OpenFolder(std::filesystem::path(path));
}
} // namespace gits::gui
