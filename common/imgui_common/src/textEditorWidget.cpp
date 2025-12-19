// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "textEditorWidget.h"

#include <fstream>

#include "log.h"

namespace gits {
namespace ImGuiHelper {

static const auto& TOOL_BAR() {
  static const std::map<TextEditorWidget::TOOL_BAR_ITEMS, gits::ImGuiHelper::ButtonGroupItem>
      items = {
          {TextEditorWidget::TOOL_BAR_ITEMS::SAVE, {"Save", "Save the current document"}},
          {TextEditorWidget::TOOL_BAR_ITEMS::REVERT,
           {"Revert", "Revert to the last saved version of the document"}},
          {TextEditorWidget::TOOL_BAR_ITEMS::UNDO, {"Undo", "Undo the last change"}},
          {TextEditorWidget::TOOL_BAR_ITEMS::REDO, {"Redo", "Redo last undone change"}},
          {TextEditorWidget::TOOL_BAR_ITEMS::CHECK, {"Check", "Check the document for errors"}},
      };
  return items;
}

static const auto& TOOL_BAR_VEC() {
  static const std::vector<std::string> values([] {
    std::vector<std::string> result;
    for (const auto& pair : TOOL_BAR()) {
      result.push_back(pair.second.label);
    }
    return result;
  }());
  return values;
}

TextEditorWidget::TextEditorWidget(std::string name)
    : m_Name(std::move(name)),
      m_Editor(),
      m_FilePath(std::nullopt),
      m_Config(TextEditorWidget::Config()) {
  m_BtnsToolBar = std::make_unique<ImGuiHelper::ButtonGroup<TOOL_BAR_ITEMS>>(TOOL_BAR());
}

TextEditorWidget::~TextEditorWidget() {
  m_BtnsToolBar.reset();
}

TextEditor& TextEditorWidget::GetEditor() {
  return m_Editor;
}

void TextEditorWidget::SetSaveCallback(std::function<void(std::filesystem::path)> callback) {
  m_OnSave = callback;
}

void TextEditorWidget::SetRevertCallback(std::function<void(std::filesystem::path)> callback) {
  m_OnRevert = callback;
}

void TextEditorWidget::SetCheckCallback(std::function<bool(const std::string&)> callback) {
  m_OnCheck = callback;
}

void TextEditorWidget::SetConfig(const Config& cfg) {
  m_Config = cfg;
}

TextEditorWidget::Config& TextEditorWidget::GetConfig() {
  return m_Config;
}

void TextEditorWidget::SetFilePath(const std::filesystem::path& path) {
  m_FilePath = path;
}
const std::filesystem::path& TextEditorWidget::GetFilePath() const {
  return m_FilePath.value();
}

void TextEditorWidget::SetText(const std::string& msg) {
  std::lock_guard<std::mutex> lock(m_EditorMutex);
  m_Editor.SetText(msg);
}

void TextEditorWidget::AppendText(const std::string& msg, bool addNewLine) {
  {
    std::lock_guard<std::mutex> lock(m_EditorMutex);
    std::string currentText = m_Editor.GetText();
    currentText += msg;
    if (addNewLine) {
      currentText += "\n";
    }
    m_Editor.SetText(currentText);
  }
  if (m_Config.ScrollToBottom) {
    ScrollToBottom();
  }
}

void TextEditorWidget::UpdatePalette() {
  const ImGuiStyle& style = ImGui::GetStyle();
  auto palette = m_Editor.GetPalette();

  auto setPaletteColor = [&](TextEditor::PaletteIndex paletteIdx, ImGuiCol colorEnum) {
    palette[(unsigned)paletteIdx] = ImGui::ColorConvertFloat4ToU32(style.Colors[colorEnum]);
  };

  // Basic text colors
  setPaletteColor(TextEditor::PaletteIndex::Default, ImGuiCol_Text);
  setPaletteColor(TextEditor::PaletteIndex::Identifier, ImGuiCol_Text);
  setPaletteColor(TextEditor::PaletteIndex::Cursor, ImGuiCol_Text);

  // Syntax highlighting colors - use more varied colors
  setPaletteColor(TextEditor::PaletteIndex::Keyword, ImGuiCol_ButtonActive);
  setPaletteColor(TextEditor::PaletteIndex::Number, ImGuiCol_PlotHistogram);
  setPaletteColor(TextEditor::PaletteIndex::String, ImGuiCol_PlotLines);
  setPaletteColor(TextEditor::PaletteIndex::CharLiteral, ImGuiCol_PlotLines);
  setPaletteColor(TextEditor::PaletteIndex::Punctuation, ImGuiCol_ButtonActive);

  // Comments - use disabled text color
  setPaletteColor(TextEditor::PaletteIndex::Comment, ImGuiCol_TextDisabled);
  setPaletteColor(TextEditor::PaletteIndex::MultiLineComment, ImGuiCol_TextDisabled);

  // Preprocessor - use a distinct color
  setPaletteColor(TextEditor::PaletteIndex::Preprocessor, ImGuiCol_ButtonHovered);

  // Background and UI elements
  setPaletteColor(TextEditor::PaletteIndex::Background, ImGuiCol_WindowBg);
  setPaletteColor(TextEditor::PaletteIndex::Selection, ImGuiCol_Header);
  setPaletteColor(TextEditor::PaletteIndex::LineNumber, ImGuiCol_TextDisabled);
  setPaletteColor(TextEditor::PaletteIndex::CurrentLineFill, ImGuiCol_FrameBg);
  setPaletteColor(TextEditor::PaletteIndex::CurrentLineFillInactive, ImGuiCol_FrameBgHovered);
  setPaletteColor(TextEditor::PaletteIndex::CurrentLineEdge, ImGuiCol_Border);

  // Error highlighting
  setPaletteColor(TextEditor::PaletteIndex::ErrorMarker, ImGuiCol_PlotHistogramHovered);

  // Breakpoint colors - create custom colors based on theme
  bool isDarkTheme = style.Colors[ImGuiCol_WindowBg].x < 0.5f;
  if (isDarkTheme) {
    palette[(unsigned)TextEditor::PaletteIndex::Breakpoint] = IM_COL32(128, 0, 0, 255); // Dark red
  } else {
    palette[(unsigned)TextEditor::PaletteIndex::Breakpoint] =
        IM_COL32(255, 100, 100, 255); // Light red
  }

  m_Editor.SetPalette(palette);
}

const std::string TextEditorWidget::GetText() const {
  return m_Editor.GetText();
}

void TextEditorWidget::SetBreakpoints(const TextEditor::Breakpoints& aMarkers) {
  std::lock_guard<std::mutex> lock(m_EditorMutex);
  m_Editor.SetBreakpoints(aMarkers);
}

void TextEditorWidget::ScrollToBottom() {
  std::lock_guard<std::mutex> lock(m_EditorMutex);
  auto lines = m_Editor.GetTotalLines();
  m_Editor.SetCursorPosition(TextEditor::Coordinates(lines, 0));
}

bool TextEditorWidget::SaveToFile(std::filesystem::path filePath) {
  std::ofstream outFile(filePath);
  if (!outFile.is_open()) {
    LOG_ERROR << "Failed to open file for saving: " << filePath;
    return false;
  }
  outFile << m_Editor.GetText();
  outFile.close();
  return true;
}

bool TextEditorWidget::LoadFromFile(std::filesystem::path filePath) {
  std::ifstream inFile(filePath);
  if (!inFile.is_open()) {
    LOG_ERROR << "Failed to open file for loading: " << filePath;
    return false;
  }
  m_FilePath = filePath;
  std::string content((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
  m_Editor.SetText(content);
  return true;
}

void TextEditorWidget::Render() {
  auto available = ImGui::GetContentRegionAvail();
  return Render(available);
}

void TextEditorWidget::Render(ImVec2 size) {
  std::lock_guard<std::mutex> lock(m_EditorMutex);
  if (m_Config.ShowToolbar) {
    RenderToolbar(size);
  }
  m_Editor.Render(m_Name.c_str());
}

void TextEditorWidget::RenderToolbar(ImVec2 size) {
  auto width = m_BtnsToolBar->GetSize().x;
  if ((ImGui::GetCursorPosX() + width + ImGui::GetStyle().WindowPadding.x) > size.x) {
    ImGui::NewLine();
  } else {
    ImGui::SetCursorPosX(size.x - width - ImGui::GetStyle().WindowPadding.x);
  }

  bool changedContent = m_Editor.IsTextChanged() && m_FilePath.has_value();

  m_BtnsToolBar->SetEnabled(TOOL_BAR_ITEMS::SAVE, changedContent);
  m_BtnsToolBar->SetEnabled(TOOL_BAR_ITEMS::REVERT, changedContent);
  m_BtnsToolBar->SetEnabled(TOOL_BAR_ITEMS::UNDO, m_Editor.CanUndo());
  m_BtnsToolBar->SetEnabled(TOOL_BAR_ITEMS::REDO, m_Editor.CanRedo());
  m_BtnsToolBar->SetEnabled(TOOL_BAR_ITEMS::CHECK, m_OnCheck != nullptr);

  if (m_BtnsToolBar->Render(true)) {
    switch (m_BtnsToolBar->Selected()) {
    case TOOL_BAR_ITEMS::SAVE:
      if (m_FilePath.has_value()) {
        if (m_OnSave) {
          m_OnSave(m_FilePath.value());
        } else {
          SaveToFile(m_FilePath.value());
        }
      } else {
        LOG_WARNING << "Save button clicked but no file path is set.";
      }
      break;
    case TOOL_BAR_ITEMS::REVERT:
      if (m_FilePath.has_value()) {
        if (m_OnRevert) {
          m_OnRevert(m_FilePath.value());
        } else {
          LoadFromFile(m_FilePath.value());
        }
      } else {
        LOG_WARNING << "Revert button clicked but no file path is set.";
      }
      break;
    case TOOL_BAR_ITEMS::UNDO:
      m_Editor.Undo();
      break;
    case TOOL_BAR_ITEMS::REDO:
      if (m_Editor.CanRedo()) {
        m_Editor.Redo();
      }
      break;
    case TOOL_BAR_ITEMS::CHECK:
      if (m_OnCheck) {
        bool result = m_OnCheck(m_Editor.GetText());
        LOG_INFO << "Check completed with result: "
                 << (result ? "No issues found." : "Issues detected.");
      }
      break;
    default:
      break;
    }
  }
}
} // namespace ImGuiHelper
} // namespace gits
