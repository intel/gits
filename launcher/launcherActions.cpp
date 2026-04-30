// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "launcherActions.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

#include "ImGuiFileDialog.h"
#include "yaml-cpp/yaml.h"

#include "fileActions.h"
#include "labels.h"
#include "captureActions.h"
#include "log.h"
#include "configurator.h"
#include "metaDataActions.h"
#include "eventBus.h"

namespace gits::gui {

typedef Context::SideBarItem SideBarItem;

bool ValidateYaml(const std::string& text) {
  try {
    YAML::Node node = YAML::Load(text);
    (void)node; // Workaround for unused variable warning
    return true;
  } catch (const YAML::Exception& e) {
    LOG_ERROR << "Error validating YAML: " << e.what();
    return false;
  }

  return true;
}

bool ValidateGITSConfig(const std::string& config) {
  const auto result = gits::Configurator::Instance().Load(config);
  if (!result) {
    LOG_ERROR << "Error validating configuration from";
    return false;
  }

  return true;
}

void UpdateCLICall() {
  auto& context = Context::GetInstance();
  context.UpdateFixedLauncherArguments();

  switch (context.AppMode) {
  case Mode::PLAYBACK:
  // [[fallthrough]]
  case Mode::SUBCAPTURE: {

    const auto gitsExecutable = context.GetGITSPlayerPath();

    context.CLIArguments.clear();

    context.CLIArguments.push_back("--config=" + context.GetPathSafe(Path::CONFIG).string());
    context.CLIArguments.push_back(context.FixedLauncherArguments);
    context.CLIArguments.push_back(context.CustomArguments);

    const auto& streamPath = context.GetPathSafe(Path::INPUT_STREAM);
    context.CLIArguments.push_back(streamPath.string());

    context.CLIArguments.erase(std::remove_if(context.CLIArguments.begin(),
                                              context.CLIArguments.end(),
                                              [](const std::string& arg) { return arg.empty(); }),
                               context.CLIArguments.end());

    break;
  }
  case Mode::CAPTURE: {

    const auto targetExecutable = context.GetPathSafe(Path::CAPTURE_TARGET);

    context.CLIArguments.clear();
    context.CLIArguments.push_back(context.CaptureCustomArguments);

    context.CLIArguments.erase(std::remove_if(context.CLIArguments.begin(),
                                              context.CLIArguments.end(),
                                              [](const std::string& arg) { return arg.empty(); }),
                               context.CLIArguments.end());

    break;
  }
  default:
    break;
  }

  EventBus::GetInstance().publish<ContextEvent>(ContextEvent::Type::CLIUpdated);
}

void PlaybackStream() {
  auto& context = Context::GetInstance();
  const auto gitsPlayerPath = context.GetGITSPlayerPath();

  context.GITSLogEditor->SetText("");

  FileActions::LaunchExecutableThreadCallbackOnExit(
      gitsPlayerPath, context.CLIArguments, gitsPlayerPath.parent_path(),
      [&context](const std::string& log) { context.GITSLog(log); },
      [&context]() {
        // Check log for errors and publish result
        const auto& logText = context.GITSLogEditor->GetText();
        bool oom = logText.find("E_OUTOFMEMORY") != std::string::npos;
        EventBus::GetInstance().publish(ActionEvent::Type::Playback, ActionEvent::State::Ended,
                                        oom ? ActionEvent::Status::Failure
                                            : ActionEvent::Status::Success,
                                        oom ? "E_OUTOFMEMORY" : "");
      });
  context.BtnsSideBar->SelectEntry(SideBarItem::LOG);
}

void SubcaptureStream() {
  auto& context = Context::GetInstance();
  const auto gitsPlayerPath = context.GetGITSPlayerPath();

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

  context.BtnsSideBar->SelectEntry(SideBarItem::LOG);
}

void GenerateCCode(CCodeExport parameters) {
  auto& context = Context::GetInstance();
  const auto gitsPlayerPath = context.GetGITSPlayerPath();

  context.GITSLogEditor->SetText("");

  //build arguments based on paramenters:
  std::vector<std::string> cliArguments;

  cliArguments.push_back("--DirectX.Player.CCode.Enabled");

  cliArguments.push_back("--DirectX.Player.CCode.CommandsPerBlock=" +
                         std::to_string(parameters.CommandsPerBlock));
  cliArguments.push_back("--DirectX.Player.CCode.WrapApiCalls.Value=" +
                         std::to_string(parameters.WrapAPICalls ? 1 : 0));
  cliArguments.push_back("--DirectX.Player.CCode.CCodePath=" + parameters.CCodePath.string());
  cliArguments.push_back(parameters.StreamPath.string());

  FileActions::LaunchExecutableThreadCallbackOnExit(
      gitsPlayerPath, cliArguments, gitsPlayerPath.parent_path(),
      [&context](const std::string& log) { context.GITSLog(log); },
      [&context]() {
        // Check log for errors and publish result
        const auto& logText = context.GITSLogEditor->GetText();
        bool oom = logText.find("E_OUTOFMEMORY") != std::string::npos;
        EventBus::GetInstance().publish(
            ActionEvent::Type::CCodeGeneration, ActionEvent::State::Ended,
            oom ? ActionEvent::Status::Failure : ActionEvent::Status::Success,
            oom ? "E_OUTOFMEMORY" : "");
      });
  context.BtnsSideBar->SelectEntry(SideBarItem::LOG);
}

void UpdateConfigSectionPositions(const std::vector<std::string>& config) {
  auto& context = Context::GetInstance();
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
    context.ConfigSectionLines[item.first] = pos;
    idx++;
  }
}

std::optional<std::string> ProcessFileDialog(std::string imGuiDialogKey) {
  std::optional<std::string> result = std::nullopt;
  const auto& parentSize = ImGui::GetWindowSize();
  const auto defaultScale = 0.75f;
  const auto minScale = 0.5f;
  const auto maxScale = 1.0f;
  ImGui::SetNextWindowSize(ImVec2(parentSize.x * defaultScale, parentSize.y * defaultScale),
                           ImGuiCond_FirstUseEver);
  if (ImGuiFileDialog::Instance()->Display(
          imGuiDialogKey, 32, ImVec2(parentSize.x * minScale, parentSize.y * minScale),
          ImVec2(parentSize.x * maxScale, parentSize.y * maxScale))) {
    if (ImGuiFileDialog::Instance()->IsOk()) {
      result = ImGuiFileDialog::Instance()->GetFilePathName();
    }
    ImGuiFileDialog::Instance()->Close();
  }
  return result;
}

void FileDialogs() {
  auto& context = Context::GetInstance();
  if (!context.CurrentFileDialogKey.has_value()) {
    return;
  }
  auto key = context.CurrentFileDialogKey.value();
  if (!ProcessFileDialog(key.ImGuiKey())) {
    return;
  }

  auto path = ImGuiFileDialog::Instance()->GetFilePathName();
  if (path.empty()) {
    path = ImGuiFileDialog::Instance()->GetCurrentPath();
  }
  if (path.empty()) {
    return;
  }

  if (key.Path == Path::GITS_BASE) {
    context.Paths.BasePath = path;
    DetectBasePaths();
    context.SendAllPathsSetEvents();
  } else if (key.Path == Path::GITS_LOG) {
    EventBus::GetInstance().publish(PathEvent(key.Path, key.Mode, path));
  } else {
    context.SetPath(path, key.Path, key.Mode);
  }
}

bool SetupFileDialogPath(Path path,
                         Mode mode,
                         IGFD::FileDialogConfig* dlgConfig,
                         std::filesystem::path customPath) {
  auto contextPath = Context::GetInstance().GetPathSafe(path, mode);
  if (!customPath.empty()) {
    contextPath = customPath;
  }
  if (contextPath.empty()) {
    dlgConfig->path = "";
  } else {
    dlgConfig->path = contextPath.string();
  }
  return true;
}

void ShowFileDialog(FileDialogKey key, std::filesystem::path path) {
  auto& context = Context::GetInstance();

  context.CurrentFileDialogKey = key;

  std::string ext;
  IGFD::FileDialogConfig dlgConfig;
  dlgConfig.flags = ImGuiFileDialogFlags_Modal;
  bool proceed = false;

  proceed = SetupFileDialogPath(key.Path, key.Mode, &dlgConfig, path);
  switch (key.Path) {
  case Path::CAPTURE_TARGET:
    ext = ".exe";
    break;
  case Path::CONFIG:
    ext = ".yml";
    break;
  case Path::INPUT_STREAM:
    ext = ".gits2";
    break;
  case Path::GITS_LOG:
    ext = ".txt";
    break;
  default:
    break;
  }

  if (!ext.empty()) {
    dlgConfig.filePathName = dlgConfig.path;
  }

  const auto title = Labels::DialogTitle(key);
  if (proceed) {
    ImGuiFileDialog::Instance()->OpenDialog(key.ImGuiKey(), title,
                                            ext.empty() ? nullptr : ext.c_str(), dlgConfig);
  }
}

void LoadConfigFile() {
  auto& context = Context::GetInstance();

  auto filePath = context.GetPathSafe(Path::CONFIG);
  if (filePath.empty()) {
    context.ConfigEditor->SetText("// No file specified.");
    return;
  }

  auto fhandle = std::ifstream(filePath);
  if (fhandle.is_open()) {
    const std::string str((std::istreambuf_iterator<char>(fhandle)),
                          std::istreambuf_iterator<char>());
    context.ConfigEditor->SetText(str);
    UpdateConfigSectionPositions(context.ConfigEditor->GetEditor().GetTextLines());
    TextEditor::Breakpoints breakpoints;
    for (const auto& section : context.ConfigSectionLines) {
      int line = section.second;
      context.BtnsAPI->SetEnabled(section.first, line >= 0);
      if (line >= 0) {
        breakpoints.insert(line + 1);
      }
    }
    context.ConfigEditor->SetFilePath(filePath);
    context.ConfigEditor->SetBreakpoints(breakpoints);
  } else {
    context.ConfigEditor->SetText("// Could not open file:\n> " + filePath.string());
  }
}

void SetImGuiStyle(size_t idx) {
  auto& context = Context::GetInstance();
  context.LauncherConfiguration.Theme.SetThemeByIdx(idx);
  context.LauncherConfiguration.Theme.ApplyTheme();

  // We do both a direct call to context function and an event
  // since not everything yet uses the events system
  context.UpdatePalette();
  EventBus::GetInstance().publish<AppEvent>(AppEvent::Type::ThemeChanged);
}

std::string GetRecorderDirectoryNameForApi(Api api) {
  const std::map<Api, std::string> recorderDirectoryForApi{{Api::UNKNOWN, ""},
                                                           {Api::DIRECTX, "FilesToCopyDirectX"},
                                                           {Api::OPENGL, "FilesToCopyOGL"},
                                                           {Api::VULKAN, "FilesToCopyVulkan"},
                                                           {Api::OPENCL, "FilesToCopyOCL"},
                                                           {Api::LEVELZERO, "FilesToCopyL0"}};

  return recorderDirectoryForApi.at(api);
}

std::filesystem::path GetRecorderConfigPathForApi(Api api) {
  return Context::GetInstance().Paths.BasePath / filesystem_names::RECORDER_DIRECTORY_NAME /
         GetRecorderDirectoryNameForApi(api) / filesystem_names::RECORDER_CONFIG_FILENAME;
}

std::filesystem::path GetPlayerConfigPath() {
  auto& context = Context::GetInstance();
  const auto playerConfigPath =
      context.Paths.BasePath / "Player" / filesystem_names::PLAYER_CONFIG_FILENAME;
  if (std::filesystem::exists(playerConfigPath)) {
    return playerConfigPath;
  }
  return "";
}

bool IsValidGITSBasePath(const std::filesystem::path& path) {
  if (path.empty()) {
    return false;
  }
#ifdef _WIN32
  const auto playerExecutablePath = path / "Player" / filesystem_names::GITS_PLAYER_WIN;
  const auto recorderDLLPath = path / "Recorder" / filesystem_names::GITS_RECORDER_WIN;
#else
  const auto playerExecutablePath = path / "Player" / filesystem_names::GITS_PLAYER_LINUX;
  const auto recorderDLLPath = path / "Recorder" / filesystem_names::GITS_RECORDER_LINUX;
#endif

  return std::filesystem::exists(playerExecutablePath) && std::filesystem::exists(recorderDLLPath);
}

void DetectBasePaths() {
  // the parent folder of the folder in which the executable being run is:
  // Get the parent folder of the folder in which the executable is being run
#ifdef _WIN32
  char exePath[MAX_PATH];
  GetModuleFileNameA(nullptr, exePath, MAX_PATH);
  std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
#else
  char exePath[PATH_MAX];
  ssize_t count = readlink("/proc/self/exe", exePath, PATH_MAX);
  std::filesystem::path exeDir =
      std::filesystem::path(std::string(exePath, (count > 0) ? count : 0)).parent_path();
#endif
  auto& context = Context::GetInstance();
  auto detected = !context.Paths.BasePath.empty() && IsValidGITSBasePath(context.Paths.BasePath);
  if (!detected) {
    detected = IsValidGITSBasePath(exeDir);
    if (detected) {
      context.Paths.BasePath = exeDir;
      LOG_DEBUG << "Detected base path from executable location: " << context.Paths.BasePath;
    }
  }
  if (!detected) {
    const auto buildDir = exeDir.parent_path().parent_path();
    const auto distDir = buildDir / "dist";
    detected = IsValidGITSBasePath(distDir);
    if (detected) {
      context.Paths.BasePath = distDir;
      LOG_DEBUG << "Detected base path from build/dist relative location: "
                << context.Paths.BasePath;
    }
  }
  if (!detected) {
    LOG_WARNING << "Could not detect GITS base path. Please set it manually.";
    context.Paths.BasePath = "";
  } else {
    const auto playerConfigPath =
        context.Paths.BasePath / "Player" / filesystem_names::PLAYER_CONFIG_FILENAME;
    if (!std::filesystem::exists(context.Paths.Playback.ConfigPath)) {
      context.Paths.Playback.ConfigPath = playerConfigPath;
    }
    if (!std::filesystem::exists(context.Paths.Subcapture.ConfigPath)) {
      context.Paths.Subcapture.ConfigPath = playerConfigPath;
    }
  }
}

void ResetBasePaths() {
  auto& context = Context::GetInstance();
  context.Paths.BasePath = "";
  DetectBasePaths();
  context.SendAllPathsSetEvents();

  UpdateCLICall();
  LoadConfigFile();
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
