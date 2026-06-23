// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <mutex>
#include <memory>
#include <unordered_set>
#include <string>

#include <imgui.h>
#include <TextEditor.h>

#include "buttonGroup.h"
#include "imGuiHelper.h"

namespace gits {
namespace ImGuiHelper {

/**
   * A Text Editor Widget with toolbar and file handling capabilities.
   */
class TextEditorWidget {

public:
  enum class TOOL_BAR_ITEMS {
    SAVE = 0,
    REVERT,
    UNDO,
    REDO,
    CHECK,
    SEND_BY_EMAIL,
    EXPORT
  };

  struct Config {
    bool ShowToolbar = true;
    bool ShowClearButton = true;
    bool ScrollToBottom = true;
    std::unordered_set<TOOL_BAR_ITEMS> ToolBarItems = {TOOL_BAR_ITEMS::SAVE, TOOL_BAR_ITEMS::REVERT,
                                                       TOOL_BAR_ITEMS::UNDO, TOOL_BAR_ITEMS::REDO};
  };
  static inline const Config CONFIG_NO_TOOLBAR =
      Config{.ShowToolbar = false,
             .ShowClearButton = false,
             .ScrollToBottom = false,
             .ToolBarItems = {TOOL_BAR_ITEMS::SAVE, TOOL_BAR_ITEMS::REVERT, TOOL_BAR_ITEMS::UNDO,
                              TOOL_BAR_ITEMS::REDO}};

  TextEditorWidget(const TextEditorWidget&) = delete;
  TextEditorWidget& operator=(const TextEditorWidget&) = delete;

  TextEditorWidget(std::string name);
  ~TextEditorWidget();

  TextEditor& GetEditor();

  // All callbacks are (currently) called from the main GUI thread. Make sure to offload heavy work to other threads if needed.
  // When no Callback is set for Save & Revert the editor will save the file directly to the previous name or load from it.
  // When a callback is set the save/revert function is _not called_, instead the callback is called with the current file path and can decide what to do.
  // The check callback should return true if the check was successful, false otherwise. Potential future updates could provide more detailed feedback, e.g. Breakpoints or error messages.
  void SetSaveCallback(std::function<bool(std::filesystem::path)> callback);
  void SetRevertCallback(std::function<bool(std::filesystem::path)> callback);
  void SetCheckCallback(std::function<std::optional<std::string>(const std::string&)> callback);
  void SetSendByEmailCallback(std::function<void(const std::string&)> callback);
  void SetExportCallback(std::function<void(const std::filesystem::path)> callback);

  static const TextEditor::LanguageDefinition& GetYamlLanguageDefinition();

  void SetConfig(const Config& cfg);
  Config& GetConfig();

  void SetFilePath(const std::filesystem::path& path);
  const std::filesystem::path& GetFilePath() const;

  void SetText(const std::string& msg);
  void AppendText(const std::string& msg, bool addNewLine = true);
  const std::string GetText() const;

  void UpdatePalette();

  void SetBreakpoints(const TextEditor::Breakpoints& aMarkers);
  void ScrollToBottom();

  bool SaveToFile(std::filesystem::path filePath);
  bool LoadFromFile(std::filesystem::path filePath);

  void Render();
  void Render(ImVec2 size);

  void SetFilterOutFirstYMLComment(bool enableFiltering);
  void SetLiveCheck(bool doLiveCheck);
  void SetShowCheckStatus(bool showCheckStatus);

private:
  void RenderToolbar(ImVec2 available);
  void RunCheck();

  std::function<bool(std::filesystem::path)> m_OnSave;
  std::function<bool(std::filesystem::path)> m_OnRevert;
  std::function<std::optional<std::string>(const std::string&)> m_OnCheck;
  std::function<void(const std::string&)> m_OnSendByEmail;
  std::function<void(std::filesystem::path)> m_OnExport;

private:
  std::string m_Name;
  TextEditor m_Editor;
  std::filesystem::path m_FilePath;
  Config m_Config;
  bool m_TextDirty = false;
  bool m_DoLiveCheck = false;
  bool m_ShowCheckStatus = false;
  mutable std::mutex m_EditorMutex;
  std::unique_ptr<ImGuiHelper::ButtonGroup<TOOL_BAR_ITEMS>> m_BtnsToolBar;
  std::optional<bool> m_LastCheckResult = std::nullopt;
  std::optional<std::string> m_LastCheckErrorMessage = std::nullopt;
  bool m_FilterOutFirstYMLComment = false;
  std::string m_HeaderCommentBlock = "";
};

} // namespace ImGuiHelper
} // namespace gits
