// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "launcherActions.h"

#include <cstdlib>
#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
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
#include "configOptions.h"

#include "configurationYAMLAuto.h"
#include <iterator>
#include <yaml-cpp/emitter.h>
#include <configurationAuto.h>

namespace gits::gui {

typedef Context::SideBarItem SideBarItem;

std::optional<std::string> ValidateYaml(const std::string& text) {
  try {
    YAML::Node node = YAML::Load(text);
    (void)node; // Workaround for unused variable warning
  } catch (const YAML::Exception& e) {
    return "Error validating YAML: " + std::string(e.what());
  }
  return std::nullopt;
}

std::optional<std::string> ValidateGITSConfig(const std::string& config) {
  return gits::Configurator::Instance().Validate(config);
}

#ifdef _WIN32
std::string QuoteWindowsPath(const std::string& path) {
  if (path.empty()) {
    return "\"\"";
  }
  std::string result = path;
  // If path ends with a backslash, we add a trailing dot
  // This way the ending quote doesn't get escaped, and when GITS Player relaunches itself, the path remains correct
  if (!result.empty() && result.back() == '\\') {
    result += '.';
  }
  return "\"" + result + "\"";
}

bool IsElevated() {
  HANDLE token = nullptr;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
    return false;
  }

  TOKEN_ELEVATION elevation = {};
  DWORD size = sizeof(TOKEN_ELEVATION);
  bool result = false;
  if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size)) {
    result = elevation.TokenIsElevated != 0;
  }

  CloseHandle(token);
  return result;
}

void RelaunchAsAdmin() {
  wchar_t path[MAX_PATH] = {};
  if (!GetModuleFileNameW(nullptr, path, MAX_PATH)) {
    return;
  }

  SHELLEXECUTEINFOW sei = {};
  sei.cbSize = sizeof(sei);
  sei.lpVerb = L"runas";
  sei.lpFile = path;
  sei.nShow = SW_NORMAL;

  if (!ShellExecuteExW(&sei)) {
    DWORD error = GetLastError();
    if (error == ERROR_CANCELLED) {
      LOG_WARNING << "[LauncherActions] User declined UAC prompt\n";
    } else {
      LOG_ERROR << "[LauncherActions] Failed to relaunch as admin: " << error << "\n";
    }
    return;
  }

  LOG_INFO << "[LauncherActions] Relaunched as admin, closing current instance\n";
  PostQuitMessage(0);
}
#endif

void UpdateCLICall() {
  auto& context = Context::GetInstance();
  context.UpdateFixedLauncherArguments();

  switch (context.AppMode) {
  case Mode::PLAYBACK:
  // [[fallthrough]]
  case Mode::SUBCAPTURE: {

    context.CLIArguments.clear();

    context.CLIArguments.push_back(context.CustomArguments);

    const auto& streamPath = context.GetPathSafe(Path::INPUT_STREAM);
#ifdef _WIN32
    context.CLIArguments.push_back(QuoteWindowsPath(streamPath.string()));
#else
    context.CLIArguments.push_back("\"" + streamPath.string() + "\" ");
#endif

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

  const auto tmpConfigInfo =
      PrepareTemporaryConfigFile(Mode::PLAYBACK, gitsPlayerPath.parent_path());
  if (tmpConfigInfo.TemporaryConfigPath.empty()) {
    LOG_ERROR << "Couldn't playback the stream, couldn't create a temporary config file";
  }
  auto argsWithTmpConfig = context.CLIArguments;

#ifdef _WIN32
  argsWithTmpConfig.push_back("--config=" +
                              QuoteWindowsPath(tmpConfigInfo.TemporaryConfigPath.string()));
#else
  argsWithTmpConfig.push_back("--config=" + tmpConfigInfo.TemporaryConfigPath.string());
#endif

  context.GITSLogEditor->SetText("");

  FileActions::LaunchExecutableThreadCallbackOnExit(
      gitsPlayerPath, argsWithTmpConfig, gitsPlayerPath.parent_path(),
      [&context](const std::string& log) { context.GITSLog(log); },
      [&context, tmpConfigInfo]() {
        // Check log for errors and publish result
        const auto& logText = context.GITSLogEditor->GetText();
        bool oom = logText.find("E_OUTOFMEMORY") != std::string::npos;
        EventBus::GetInstance().publish(ActionEvent::Type::Playback, ActionEvent::State::Ended,
                                        oom ? ActionEvent::Status::Failure
                                            : ActionEvent::Status::Success,
                                        oom ? "E_OUTOFMEMORY" : "");
        std::filesystem::remove(tmpConfigInfo.TemporaryConfigPath);
      });
  context.BtnsSideBar->SelectEntry(SideBarItem::LOG);
}

void SubcaptureStream() {
  auto& context = Context::GetInstance();
  const auto gitsPlayerPath = context.GetGITSPlayerPath();

  const auto tmpConfigInfo =
      PrepareTemporaryConfigFile(Mode::SUBCAPTURE, gitsPlayerPath.parent_path());
  if (tmpConfigInfo.TemporaryConfigPath.empty()) {
    LOG_ERROR << "Couldn't playback the stream, couldn't create a temporary config file";
  }
  auto argsWithTmpConfig = context.CLIArguments;
#ifdef _WIN32
  argsWithTmpConfig.push_back("--config=" +
                              QuoteWindowsPath(tmpConfigInfo.TemporaryConfigPath.string()));
#else
  argsWithTmpConfig.push_back("--config=" + tmpConfigInfo.TemporaryConfigPath.string());
#endif

  context.GITSLogEditor->SetText("");
  // TODO: Create a generic solution for this
  std::thread([gitsPlayerPath, &context, tmpConfigInfo, argsWithTmpConfig]() {
    context.SubcaptureInProgress = true;
    // since we need to run player twice, consecutively & blocking, we do it in a separate thread
    FileActions::LaunchExecutable(gitsPlayerPath, argsWithTmpConfig, true,
                                  gitsPlayerPath.parent_path(),
                                  [&context](const std::string& log) { context.GITSLog(log); });
    FileActions::LaunchExecutable(gitsPlayerPath, argsWithTmpConfig, true,
                                  gitsPlayerPath.parent_path(),
                                  [&context](const std::string& log) { context.GITSLog(log); });
    std::filesystem::remove(tmpConfigInfo.TemporaryConfigPath);
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
  } else if (key.Path == Path::CONFIG_EXPORT) {
    EventBus::GetInstance().publish(PathEvent(key.Path, key.Mode, path));
  } else {
    context.SetPath(path, key.Path, key.Mode);
  }
}

bool SetupFileDialogPath(Path path,
                         Mode mode,
                         IGFD::FileDialogConfig* dlgConfig,
                         const std::filesystem::path& customPath) {
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

void ShowFileDialog(FileDialogKey key, const std::filesystem::path& path) {
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
  case Path::CONFIG_EXPORT:
    ext = ".yml";
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

TemporaryConfigInfo PrepareTemporaryConfigFile(Mode mode, const std::filesystem::path& directory) {
  auto& context = Context::GetInstance();

  if (directory.empty()) {
    return TemporaryConfigInfo();
  }

  const auto& configurationForMode = context.ConfigurationForMode(mode);

  if (mode == Mode::CAPTURE) {
    auto& dumpPath = config_options::DumpPath(mode);
    if (!dumpPath.string().ends_with("%n%_%p%")) {
      dumpPath = dumpPath / "%n%_%p%";
    }
  }
  if (mode == Mode::SUBCAPTURE) {
    auto& subcapturePath = config_options::SubcapturePath(mode);
    if (!subcapturePath.string().ends_with("%f%_%r%")) {
      subcapturePath = subcapturePath / "%f%_%r%";
    }
  }

  const auto tmpConfigPath =
      directory /
      (mode == Mode::CAPTURE
           ? filesystem_names::RECORDER_CONFIG_FILENAME
           : filesystem_names::
                 PLAYER_TEMPORARY_CONFIG_FILENAME); // For Playback and Subcapture, we can use the --config argument to pass that temporary file so we can have an arbitrary name for it
  auto originalNeedsRestoring = false;
  const std::filesystem::path originalBackupPath = directory / "gits_config_backup.yml";
  if (mode == Mode::CAPTURE && std::filesystem::exists(tmpConfigPath)) {
    originalNeedsRestoring = true;
    std::filesystem::copy_file(tmpConfigPath, originalBackupPath);
  }

  std::ofstream file(tmpConfigPath);
  if (file.is_open()) {
    file << GetYamlStringFromConfig(configurationForMode);
    file.close();
  } else {
    return TemporaryConfigInfo();
  }

  return {tmpConfigPath, originalNeedsRestoring, originalNeedsRestoring ? originalBackupPath : ""};
}

void LoadConfigFile(Mode mode) {
  auto& context = Context::GetInstance();
  auto& configurationForMode = context.ConfigurationForMode(mode);

  auto filePath = context.GetPathSafe(Path::CONFIG, mode);
  if (filePath.empty()) {
    configurationForMode.BaseGitsConfigurationStr = "// No file specified.";
    return;
  }

  if (!std::filesystem::exists(filePath)) {

    configurationForMode.BaseGitsConfigurationStr =
        "File:\n> " + filePath.string() + "\ndoesn't exist";
    return;
  }

  auto fhandle = std::ifstream(filePath);
  if (fhandle.is_open()) {
    const std::string str((std::istreambuf_iterator<char>(fhandle)),
                          std::istreambuf_iterator<char>());
    YAML::Node yaml;
    try {
      yaml = YAML::Load(str);
    } catch (const YAML::Exception& e) {
      configurationForMode.BaseGitsConfigurationStr =
          "Couldn't load configuration YAML from file:\n>" + filePath.string() + "\nError:\n" +
          e.what();
      return;
    }
    if (gits::Configurator::LoadInto(yaml, &configurationForMode.BaseGitsConfiguration)) {
      std::optional<YAML::Node> baseOverrides = std::nullopt;
      if (yaml["Overrides"]) {
        baseOverrides = YAML::Clone(yaml["Overrides"]);
        configurationForMode.BaseOverrides = baseOverrides.value();
        configurationForMode.ModifiedOverrides = YAML::Clone(yaml["Overrides"]);
      }
      configurationForMode.BaseGitsConfigurationStr =
          GetYamlStringFromConfig(configurationForMode.BaseGitsConfiguration, baseOverrides);
      configurationForMode.ModifiedGitsConfiguration = configurationForMode.BaseGitsConfiguration;
      context.UpdateInMemoryConfig(mode);
    } else {
      configurationForMode.BaseGitsConfigurationStr =
          "Couldn't deserialize configuration from file:\n>" + filePath.string();
    }
  } else {
    configurationForMode.BaseGitsConfigurationStr =
        "// Could not open file:\n> " + filePath.string();
  }

  config_options::SetLauncherDefaults(mode);
  EventBus::GetInstance().publish(ContextEvent::Type::ConfigFileLoaded, mode);
}

void LoadConfigFromMemory(Mode mode) {
  auto& context = Context::GetInstance();
  auto& configurationForMode = context.ConfigurationForMode(mode);

  configurationForMode.ModifiedGitsConfigurationStr = configurationForMode.BaseGitsConfigurationStr;
}

std::string GetYamlStringFromConfig(const gits::Configuration& config,
                                    std::optional<YAML::Node> overrides) {
  YAML::Emitter emitter;
  bool result = gits::Configurator::Emit(emitter, config, true, overrides);

  if (!result) {
    LOG_ERROR << "Couldn't get YAML for the current configuration";
    return "";
  }
  return std::string(emitter.c_str());
}

std::string GetYamlStringFromConfig(const Context::ModeConfiguration& config) {
  return GetYamlStringFromConfig(config.ModifiedGitsConfiguration, config.ModifiedOverrides);
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

bool DetectBasePaths() {
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
    auto possibleDistFoldernames = {"dist",      "install",      "dist_debug",
                                    "distDebug", "dist_release", "distRelease"};
    for (const auto& distFoldername : possibleDistFoldernames) {
      const auto distDir = buildDir / distFoldername;
      detected = IsValidGITSBasePath(distDir);
      if (detected) {
        context.Paths.BasePath = distDir;
        LOG_DEBUG << "Detected base path from build/dist relative location: "
                  << context.Paths.BasePath;
        break;
      }
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

  return detected;
}

void ResetBasePaths() {
  auto& context = Context::GetInstance();
  context.Paths.BasePath = "";
  DetectBasePaths();
  context.SendAllPathsSetEvents();

  UpdateCLICall();
  LoadConfigFile(context.AppMode);
}

void SetAllConfigsFromBasePath() {
  auto& context = Context::GetInstance();

  auto captureConfigPath = GetRecorderConfigPathForApi(context.SelectedApiForCapture);
  if (std::filesystem::exists(captureConfigPath)) {
    context.SetPath(std::move(captureConfigPath), Path::CONFIG, Mode::CAPTURE);
  }
  auto playbackConfigPath = GetPlayerConfigPath();
  if (std::filesystem::exists(playbackConfigPath)) {
    context.SetPath(playbackConfigPath, Path::CONFIG, Mode::PLAYBACK);
    context.SetPath(std::move(playbackConfigPath), Path::CONFIG, Mode::SUBCAPTURE);
  }
}

void NewLauncherSession() {
  auto& context = Context::GetInstance();

  const auto emptyPath = "";

  SetAllConfigsFromBasePath();

  context.SetPath(emptyPath, Path::CAPTURE_TARGET, Mode::CAPTURE);
  context.SetPath(emptyPath, Path::OUTPUT_STREAM, Mode::CAPTURE);

  context.SetPath(emptyPath, Path::INPUT_STREAM, Mode::PLAYBACK);
  context.SetPath(emptyPath, Path::SCREENSHOTS, Mode::PLAYBACK);
  context.SetPath(emptyPath, Path::TRACE, Mode::PLAYBACK);

  context.SetPath(emptyPath, Path::INPUT_STREAM, Mode::SUBCAPTURE);
  context.SetPath(emptyPath, Path::OUTPUT_STREAM, Mode::SUBCAPTURE);
}

void SetTracePathFromInputStream() {
  auto& context = Context::GetInstance();
  auto streamPath = context.GetPathSafe(Path::INPUT_STREAM, Mode::PLAYBACK);
  context.SetPath(streamPath.parent_path(), Path::TRACE, Mode::PLAYBACK);
}

void SetTracePathFromTargetExecutable() {
  auto& context = Context::GetInstance();
  auto targetPath = context.GetPathSafe(Path::CAPTURE_TARGET, Mode::CAPTURE);
  context.SetPath(targetPath.parent_path(), Path::TRACE, Mode::CAPTURE);
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

std::string CreateEmailBodyWithLog(const std::string& logText) {
  return std::string(Labels::EMAIL_LOG_BODY_HEADER) + logText;
}

void SendEmail(const std::string& recipient, const std::string& subject, const std::string& body) {
  auto urlEncode = [](const std::string& str) -> std::string {
    std::string encoded;
    encoded.reserve(str.size() * 3);
    for (unsigned char c : str) {
      if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' ||
          c == '_' || c == '.' || c == '~') {
        encoded += static_cast<char>(c);
      } else if (c == ' ') {
        encoded += "%20";
      } else if (c == '\n') {
        encoded += "%0A";
      } else if (c == '\r') {
      } else {
        char buf[4];
        snprintf(buf, sizeof(buf), "%%%02X", c);
        encoded += buf;
      }
    }
    return encoded;
  };

  std::string mailto =
      "mailto:" + recipient + "?subject=" + urlEncode(subject) + "&body=" + urlEncode(body);
#ifdef _WIN32
  ShellExecuteA(nullptr, "open", mailto.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
  std::string command = "xdg-open \"" + mailto + "\"";
  if (std::system(command.c_str()) != 0) {
    LOG_WARNING << "Failed to open email client via xdg-open";
  }
#endif
}

void SendLogByEmail(const std::string& recipient,
                    const std::string& subject,
                    const std::string& body) {
  SendEmail(recipient, subject, body);
  LOG_INFO << "Opening email client to send log to " << Labels::EMAIL_LOG_RECIPIENT;
}

void RestoreOriginalConfigIfNeeded(const TemporaryConfigInfo& tmpConfigInfo) {
  if (!tmpConfigInfo.OriginalNeedsRestoring) {
    return;
  }
  if (tmpConfigInfo.OriginalBackupPath.empty()) {
    LOG_ERROR << "Couldn't restore original config file. Path to the backup file is empty";
    return;
  }
  if (tmpConfigInfo.TemporaryConfigPath.empty()) {
    LOG_ERROR << "Couldn't restore original config file. Path to the temporary file is empty";
    return;
  }
  if (!std::filesystem::exists(tmpConfigInfo.OriginalBackupPath)) {
    LOG_ERROR << "Couldn't restore original config file. Backup file does not exist: "
              << tmpConfigInfo.OriginalBackupPath;
    return;
  }
  try {
    std::filesystem::copy_file(tmpConfigInfo.OriginalBackupPath, tmpConfigInfo.TemporaryConfigPath,
                               std::filesystem::copy_options::overwrite_existing);
    std::filesystem::remove(tmpConfigInfo.OriginalBackupPath);
  } catch (const std::filesystem::filesystem_error& e) {
    LOG_ERROR << "Couldn't restore original config file. Error: " << e.what();
  } catch (const std::exception& e) {
    LOG_ERROR << "Couldn't restore original config file. Error " << e.what();
  } catch (...) {
    LOG_ERROR << "Couldn't restore original config file. Unknown error occurred.";
  }
}
} // namespace gits::gui
