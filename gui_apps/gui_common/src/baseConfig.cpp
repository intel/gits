// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "baseConfig.h"
#include "baseContext.h"

#include "log.h"

namespace {
// anonymous namespace for YAML keys to avoid typos and for easier refactoring
static constexpr const char* WINDOW_SIZE_KEY = "windowSize";
static constexpr const char* WINDOW_POS_KEY = "windowPos";
static constexpr const char* UI_SCALE_KEY = "uiScale";
static constexpr const char* THEME_KEY = "theme";
static constexpr const char* RECENT_STREAMS_KEY = "recentStreams";
static constexpr const char* RECENT_TARGETS_KEY = "recentTargets";
} // namespace

namespace gits::gui {
BaseConfig::BaseConfig(const std::string appName) : AppName(std::move(appName)) {
  m_RecentStreams = {};
  m_RecentTargets = {};

  WindowPos = ImVec2(::Settings::WINDOW_POS_X, ::Settings::WINDOW_POS_Y);
  WindowSize = ImVec2(::Settings::WINDOW_SIZE_WIDTH, ::Settings::WINDOW_SIZE_HEIGHT);

  if (AppName.empty()) {
    LOG_ERROR << "AppName cannot be empty. Please provide a valid application name.";
    AppName = "gitsGUIApp";
  }

  FileLocation = GetGITSBaseConfigPath();
  UIScale = 1.0f;
}

std::filesystem::path BaseConfig::GetGITSBaseConfigPath() {
  std::filesystem::path home;

#ifdef _WIN32
  char* appData = getenv("APPDATA");
  if (appData) {
    home = std::filesystem::path(appData) / std::filesystem::path(AppName);
  }
#else
  char* homeDir = getenv("HOME");
  if (homeDir) {
    home = std::filesystem::path(homeDir) / std::filesystem::path(".config") / AppName;
  }
#endif

  try {
    std::filesystem::create_directories(home);
  } catch (const std::exception& e) {
    LOG_ERROR << "Couldn't create directory for launcher config: " << home
              << " Error: " << e.what();

    return std::filesystem::path();
  }
  return home / "gitsBaseConfig.yaml";
}

BaseConfig BaseConfig::FromFile(const std::string appName, const std::filesystem::path& path) {
  BaseConfig config(appName);
  config.FillFromFile(path);
  return config;
}

// Return by value (avoid returning reference to local)
void BaseConfig::FillFromFile(const std::filesystem::path& path) {
  if (!path.empty()) {
    FileLocation = path;
  }

  const auto& guiConfigPath = FileLocation;
  if (!std::filesystem::exists(guiConfigPath)) {
    return;
  }

  try {
    std::ifstream file(guiConfigPath);
    if (!file.is_open()) {
      return;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    YAML::Node yaml = YAML::Load(content);
    ReadCustom(yaml); // Allow derived classes to read their own custom settings
    auto paths = AppPaths();
    paths.Read(yaml);
    BaseContext::GetBaseInstance().SetAppPaths(paths);
    if (yaml[WINDOW_SIZE_KEY]) {
      const auto sizeNode = yaml[WINDOW_SIZE_KEY];
      if (sizeNode.IsSequence() && sizeNode.size() == 2) {
        WindowSize = ImVec2(sizeNode[0].as<double>(), sizeNode[1].as<double>());
      }
    }
    if (yaml[WINDOW_POS_KEY]) {
      const auto posNode = yaml[WINDOW_POS_KEY];
      if (posNode.IsSequence() && posNode.size() == 2) {
        WindowPos = ImVec2(posNode[0].as<double>(), posNode[1].as<double>());
      }
    }
    if (yaml[UI_SCALE_KEY]) {
      UIScale = yaml[UI_SCALE_KEY].as<float>();
    }
    if (yaml[THEME_KEY]) {
      const auto themeID = yaml[THEME_KEY].as<std::string>();
      if (!themeID.empty()) {
        Theme.SetThemeByID(themeID);
      }
    }
    if (yaml[RECENT_STREAMS_KEY]) {
      const auto recentStreamsNode = yaml[RECENT_STREAMS_KEY];
      if (recentStreamsNode.IsSequence()) {
        m_RecentStreams.clear();
        for (const auto& streamNode : recentStreamsNode) {
          m_RecentStreams.emplace_back(streamNode.as<std::string>());
        }
      }
    }
    if (yaml[RECENT_TARGETS_KEY]) {
      const auto recentTargetsNode = yaml[RECENT_TARGETS_KEY];
      if (recentTargetsNode.IsSequence()) {
        m_RecentTargets.clear();
        for (const auto& targetNode : recentTargetsNode) {
          m_RecentTargets.emplace_back(targetNode.as<std::string>());
        }
      }
    }

  } catch (const std::exception& e) {
    LOG_ERROR << "Error loading GUI config: " << e.what() << std::endl;
  }
}

bool BaseConfig::ToFile(const std::filesystem::path& path) {
  if (!path.empty()) {
    FileLocation = path;
  }
  const auto& guiConfigPath = FileLocation;

  try {
    std::filesystem::create_directories(guiConfigPath.parent_path());

    YAML::Emitter out;
    out << YAML::BeginMap;
    WriteCustom(out); // Allow derived classes to write their own custom settings
    const auto& paths = BaseContext::GetBaseInstance().GetAppPaths();
    paths.Write(out);

    out << YAML::Key << WINDOW_SIZE_KEY << YAML::Value;
    out << YAML::BeginSeq << WindowSize.x << WindowSize.y << YAML::EndSeq;
    out << YAML::Key << WINDOW_POS_KEY << YAML::Value;
    out << YAML::BeginSeq << WindowPos.x << WindowPos.y << YAML::EndSeq;
    out << YAML::Key << UI_SCALE_KEY << YAML::Value << UIScale;
    out << YAML::Key << THEME_KEY << YAML::Value << Theme.CurThemeID();
    out << YAML::Key << RECENT_STREAMS_KEY << YAML::Value;
    out << YAML::BeginSeq;
    for (const auto& stream : m_RecentStreams) {
      out << stream.string();
    }
    out << YAML::EndSeq;
    out << YAML::Key << RECENT_TARGETS_KEY << YAML::Value;
    out << YAML::BeginSeq;
    for (const auto& target : m_RecentTargets) {
      out << target.string();
    }
    out << YAML::EndSeq;
    out << YAML::EndMap;

    std::ofstream file(guiConfigPath);
    if (!file.is_open()) {
      return false;
    }

    file << out.c_str();
    file.close();

    return true;

  } catch (const std::exception& e) {
    LOG_ERROR << "Error saving launcher config: " << e.what() << std::endl;
    return false;
  }
}

void AddToRecentList(std::vector<std::filesystem::path>& recentList,
                     const std::filesystem::path& newPath) {
  recentList.erase(std::remove(recentList.begin(), recentList.end(), newPath), recentList.end());
  recentList.insert(recentList.begin(), newPath);
  if (recentList.size() > BaseConfig::MAX_RECENT_ENTRIES) {
    recentList.resize(BaseConfig::MAX_RECENT_ENTRIES);
  }
}

void BaseConfig::AddRecentStream(const std::filesystem::path& streamPath) {
  AddToRecentList(m_RecentStreams, streamPath);
}

const std::vector<std::filesystem::path>& BaseConfig::RecentStreams() {
  return m_RecentStreams;
}

void BaseConfig::AddRecentTarget(const std::filesystem::path& targetPath) {
  AddToRecentList(m_RecentTargets, targetPath);
}

const std::vector<std::filesystem::path>& BaseConfig::RecentTargets() {
  return m_RecentTargets;
}

} // namespace gits::gui
