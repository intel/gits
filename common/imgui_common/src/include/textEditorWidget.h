// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <mutex>

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
    COUNT
  };

  struct Config {
    bool ShowToolbar = true;
    bool ShowClearButton = true;
    bool ScrollToBottom = true;
  };

  TextEditorWidget(const TextEditorWidget&) = delete;

  TextEditorWidget(std::string name);
  ~TextEditorWidget();

  TextEditor& GetEditor();

  // All callbacks are (currently) called from the main GUI thread. Make sure to offload heavy work to other threads if needed.
  // When no Callback is set for Save & Revert the editor will save the file directly to the previous name or load from it.
  // When a callback is set the save/revert function is _not called_, instead the callback is called with the current file path and can decide what to do.
  // The check callback should return true if the check was successful, false otherwise. Potential future updates could provide more detailed feedback, e.g. Breakpoints or error messages.
  void SetSaveCallback(std::function<void(std::filesystem::path)> callback);
  void SetRevertCallback(std::function<void(std::filesystem::path)> callback);
  void SetCheckCallback(std::function<bool(const std::string&)> callback);

  void SetConfig(const Config& cfg);
  Config& GetConfig();

  void SetFilePath(const std::filesystem::path& path);
  const std::filesystem::path& GetFilePath() const;

  void SetText(const std::string& msg);
  void AppendText(const std::string& msg, bool addNewLine = true);
  const std::string GetText() const;

  void SetBreakpoints(const TextEditor::Breakpoints& aMarkers);
  void ScrollToBottom();

  bool SaveToFile(std::filesystem::path filePath);
  bool LoadFromFile(std::filesystem::path filePath);

  void Render();
  void Render(ImVec2 size);

private:
  void RenderToolbar(ImVec2 available);

  std::function<void(std::filesystem::path)> m_OnSave;
  std::function<void(std::filesystem::path)> m_OnRevert;
  std::function<bool(const std::string&)> m_OnCheck;

private:
  std::string m_Name;
  TextEditor m_Editor;
  std::optional<std::filesystem::path> m_FilePath;
  Config m_Config;
  mutable std::mutex m_EditorMutex;
  ImGuiHelper::ButtonGroup<TOOL_BAR_ITEMS>* m_BtnsToolBar;
};

} // namespace ImGuiHelper
} // namespace gits
