// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "systemSetupPanel.h"

#include <imgui.h>
#include <filesystem>
#include <system_error>
#include <vector>
#include <shlobj.h>

#include "imGuiHelper.h"
#include "context.h"
#include "launcherActions.h"
#include "labels.h"
#include "log.h"
#include "registryManager.h"

namespace gits::gui {

SystemSetupPanel::SystemSetupPanel() {}

void SystemSetupPanel::Render() {
  ImGui::SeparatorText(Labels::SYSTEM_SETUP_REGISTRY_KEYS_SEPARATOR);
  RenderRegistry();
  ImGui::SeparatorText(Labels::SYSTEM_SETUP_SHADER_CACHE_SEPARATOR);
  RenderClearShaderCacheButton();
}

void SystemSetupPanel::RenderRegistry() {
  const bool elevated = Context::GetInstance().IsAdminMode;

  if (!elevated) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImGuiHelper::Colors::WARNING);
    ImGui::Text(Labels::SYSTEM_SETUP_ADMIN_REQUIRED_TEXT);
    ImGui::PopStyleColor();
    if (ImGui::Button(Labels::SYSTEM_SETUP_RELAUNCH_AS_ADMIN_BUTTON)) {
      RelaunchAsAdmin();
    }
    ImGui::Spacing();
  }

  ImGui::BeginDisabled(!elevated);
  RenderRegistryEntries();
  ImGui::Spacing();
  RenderRegistryButtons();
  ImGui::EndDisabled();
}

void SystemSetupPanel::RenderRegistryEntries() {
  auto& manager = RegistryManager::Instance();
  const auto& entries = manager.Entries();

  for (size_t i = 0; i < entries.size(); ++i) {
    const auto& entry = entries[i];

    bool enabled = entry.Enabled;
    if (ImGui::Checkbox(entry.Label.c_str(), &enabled)) {
      manager.SetEntryEnabled(i, enabled);
    }

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      if (!entry.Remark.empty()) {
        ImGui::TextWrapped("%s", entry.Remark.c_str());
        ImGui::Separator();
      }
      for (const auto& kv : entry.KeyValues) {
        ImGui::Text(Labels::REGISTRY_TOOLTIP_KEY_PATH_FORMAT, kv.KeyPath.c_str());
        ImGui::Text(Labels::REGISTRY_TOOLTIP_VALUE_FORMAT, kv.ValueName.c_str(),
                    RegistryManager::RegValueToString(kv.Value).c_str());
        if (!kv.Remark.empty()) {
          ImGui::TextWrapped("%s", kv.Remark.c_str());
        }
      }
      ImGui::EndTooltip();
    }
  }
}

void SystemSetupPanel::RenderRegistryButtons() {
  auto& manager = RegistryManager::Instance();
  const auto& entries = manager.Entries();

  if (ImGui::Button(Labels::RESYNC_REGISTRY_BUTTON)) {
    manager.SyncEntries();
  }

  ImGui::SameLine();

  ImGuiHelper::PushButtonStyle(ImGuiHelper::ButtonStyle::Failure);
  if (ImGui::Button(Labels::CLEAR_ALL_REGISTRY_KEYS_BUTTON)) {
    manager.ClearAll();
  }
  ImGuiHelper::PopButtonStyle();

  int activeCount = 0;
  for (const auto& e : entries) {
    if (e.Enabled) {
      activeCount++;
    }
  }

  ImGui::SameLine();
  ImGui::TextDisabled(Labels::REGISTRY_ACTIVE_COUNT_FORMAT, activeCount, (int)entries.size());
}

void SystemSetupPanel::RenderClearShaderCacheButton() {
  ImGuiHelper::PushButtonStyle(ImGuiHelper::ButtonStyle::Failure);
  if (ImGui::Button(Labels::CLEAR_SHADER_CACHE_BUTTON)) {
    // Get the LocalAppDataLow folder path using Windows API
    PWSTR pszPath = nullptr;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_LocalAppDataLow, 0, nullptr, &pszPath);
    if (FAILED(hr)) {
      LOG_ERROR << Labels::LOG_LOCAL_APPDATA_LOW_FAILURE;
      ImGuiHelper::PopButtonStyle();
      return;
    }

    m_ShaderCachePath = (std::filesystem::path(pszPath) / Labels::SHADER_CACHE_PATH_INTEL_DIR /
                         Labels::SHADER_CACHE_PATH_CACHE_DIR)
                            .string();
    CoTaskMemFree(pszPath);
    m_ShowClearShaderCacheConfirm = true;
    ImGui::OpenPopup(Labels::CLEAR_SHADER_CACHE_CONFIRM_POPUP_ID);
  }
  ImGuiHelper::PopButtonStyle();

  // Render confirmation popup
  ImVec2 viewportCenter = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(viewportCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImVec2 viewportSize = ImGui::GetMainViewport()->Size;
  float minWidth = viewportSize.x * 0.4f;
  ImGui::SetNextWindowSizeConstraints(ImVec2(minWidth, 0), ImVec2(FLT_MAX, FLT_MAX));
  if (ImGui::BeginPopupModal(Labels::CLEAR_SHADER_CACHE_CONFIRM_POPUP_ID,
                             &m_ShowClearShaderCacheConfirm)) {
    ImGui::TextWrapped(Labels::CLEAR_SHADER_CACHE_CONFIRM_TEXT, m_ShaderCachePath.c_str());

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    float buttonWidth = 120.0f;
    float availableWidth = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX(availableWidth - (buttonWidth * 2 + ImGui::GetStyle().ItemSpacing.x));

    if (ImGui::Button(Labels::CLEAR_SHADER_CACHE_CONFIRM_YES_BUTTON, ImVec2(buttonWidth, 0))) {
      try {
        if (std::filesystem::exists(m_ShaderCachePath)) {
          std::error_code ec;
          size_t removedCount = 0;
          size_t skippedCount = 0;
          std::vector<std::filesystem::path> entries;

          std::filesystem::recursive_directory_iterator it(
              m_ShaderCachePath, std::filesystem::directory_options::skip_permission_denied, ec);
          std::filesystem::recursive_directory_iterator end;
          for (; it != end; it.increment(ec)) {
            if (ec) {
              LOG_ERROR << Labels::LOG_SHADER_CACHE_ITERATION_ERROR_PREFIX << ec.message()
                        << Labels::LOG_SHADER_CACHE_ITERATION_ERROR_SUFFIX;
              ec.clear();
              continue;
            }
            entries.push_back(it->path());
          }

          for (auto rit = entries.rbegin(); rit != entries.rend(); ++rit) {
            std::error_code removeEc;
            if (std::filesystem::remove(*rit, removeEc)) {
              ++removedCount;
            } else if (removeEc) {
              ++skippedCount;
            }
          }

          std::error_code rootRemoveEc;
          if (std::filesystem::remove(m_ShaderCachePath, rootRemoveEc)) {
            ++removedCount;
          } else if (rootRemoveEc) {
            ++skippedCount;
          }

          if (skippedCount == 0) {
            LOG_INFO << Labels::LOG_SHADER_CACHE_CLEARED_PREFIX << m_ShaderCachePath
                     << Labels::LOG_SHADER_CACHE_REMOVED_PREFIX << removedCount
                     << Labels::LOG_SHADER_CACHE_REMOVED_SUFFIX;
          } else {
            LOG_WARNING << Labels::LOG_SHADER_CACHE_PARTIAL_PREFIX << m_ShaderCachePath
                        << Labels::LOG_SHADER_CACHE_PARTIAL_MIDDLE << removedCount
                        << Labels::LOG_SHADER_CACHE_PARTIAL_SKIPPED_MIDDLE << skippedCount
                        << Labels::LOG_SHADER_CACHE_PARTIAL_SUFFIX;
          }
        } else {
          LOG_INFO << Labels::LOG_SHADER_CACHE_PATH_MISSING_PREFIX << m_ShaderCachePath
                   << Labels::LOG_SHADER_CACHE_PATH_MISSING_SUFFIX;
        }
      } catch (const std::exception& e) {
        LOG_ERROR << Labels::LOG_SHADER_CACHE_CLEAR_FAILURE_PREFIX << e.what()
                  << Labels::LOG_SHADER_CACHE_CLEAR_FAILURE_SUFFIX;
      }
      m_ShowClearShaderCacheConfirm = false;
      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button(Labels::CLEAR_SHADER_CACHE_CONFIRM_NO_BUTTON, ImVec2(buttonWidth, 0))) {
      m_ShowClearShaderCacheConfirm = false;
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
}

} // namespace gits::gui
