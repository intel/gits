// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "textEditorWidget.h"

#include <fstream>
#include <cstdlib>

#include "log.h"
#include <plog/Log.h>
#include <string>
#include <vector>
#include <utility>
#include <memory>
#include "buttonGroup.h"
#include <TextEditor.h>
#include <iterator>
#include <map>
#include "baseButtonGroup.h"

namespace {
bool IsBlankLine(const std::string& line) {
  return line.find_first_not_of(" \t") == std::string::npos;
}

bool IsCommentLine(const std::string& line) {
  size_t start = line.find_first_not_of(" \t");
  if (start == std::string::npos) {
    return false; // Blank line
  }
  return line[start] == '#';
}
} // namespace

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
          {TextEditorWidget::TOOL_BAR_ITEMS::SEND_BY_EMAIL, {"Send by email", "Send by email"}},
          {TextEditorWidget::TOOL_BAR_ITEMS::EXPORT,
           {"Export", "Export the document into a file"}}};
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
    : m_Name(std::move(name)), m_Editor(), m_FilePath("") {
  m_BtnsToolBar = std::make_unique<ImGuiHelper::ButtonGroup<TOOL_BAR_ITEMS>>(TOOL_BAR());
  SetConfig(TextEditorWidget::Config());
}

TextEditorWidget::~TextEditorWidget() {
  m_BtnsToolBar.reset();
}

TextEditor& TextEditorWidget::GetEditor() {
  return m_Editor;
}

void TextEditorWidget::SetSaveCallback(std::function<bool(std::filesystem::path)> callback) {
  m_OnSave = std::move(callback);
}

void TextEditorWidget::SetRevertCallback(std::function<bool(std::filesystem::path)> callback) {
  m_OnRevert = std::move(callback);
}

void TextEditorWidget::SetCheckCallback(
    std::function<std::optional<std::string>(const std::string&)> callback) {
  m_OnCheck = std::move(callback);
}

void TextEditorWidget::SetExportCallback(
    std::function<void(const std::filesystem::path)> callback) {
  m_OnExport = std::move(callback);
}

const TextEditor::LanguageDefinition& TextEditorWidget::GetYamlLanguageDefinition() {
  static bool inited = false;
  static TextEditor::LanguageDefinition langDef;
  if (!inited) {
    // YAML keywords and special values
    static const char* const keywords[] = {
        "true", "false", "null", "True", "False", "Null", "TRUE", "FALSE", "NULL", "yes", "no",
        "Yes",  "No",    "YES",  "NO",   "on",    "off",  "On",   "Off",   "ON",   "OFF"};

    for (auto& k : keywords) {
      langDef.mKeywords.insert(k);
    }

    // Minimal custom tokenizer - keys, keywords, numbers vs everything else
    langDef.mTokenize = [](const char* in_begin, const char* in_end, const char*& out_begin,
                           const char*& out_end, TextEditor::PaletteIndex& paletteIndex) -> bool {
      paletteIndex = TextEditor::PaletteIndex::Max;

      // Skip whitespace
      while (in_begin < in_end && (*in_begin == ' ' || *in_begin == '\t')) {
        in_begin++;
      }

      if (in_begin == in_end) {
        out_begin = in_end;
        out_end = in_end;
        paletteIndex = TextEditor::PaletteIndex::Default;
        return true;
      }

      // Handle YAML list brackets
      if (*in_begin == '[' || *in_begin == ']') {
        out_begin = in_begin;
        out_end = in_begin + 1;
        paletteIndex = TextEditor::PaletteIndex::Punctuation;
        return true;
      }

      // Handle YAML list separator
      if (*in_begin == ',') {
        out_begin = in_begin;
        out_end = in_begin + 1;
        paletteIndex = TextEditor::PaletteIndex::Punctuation;
        return true;
      }

      const char* p = in_begin;

      // Find the end of the current token, but include colons that are part of paths
      // Stop at brackets and commas since they're YAML syntax
      while (p < in_end && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r' && *p != '#' &&
             *p != '[' && *p != ']' && *p != ',') {
        if (*p == ':') {
          // Check if this colon is followed by whitespace (YAML key separator)
          if (p + 1 >= in_end || *(p + 1) == ' ' || *(p + 1) == '\t' || *(p + 1) == '\n' ||
              *(p + 1) == '\r') {
            // This is a YAML key separator, stop here
            break;
          }
          // Otherwise, include the colon as part of the token (for paths like C:\)
        }
        p++;
      }

      // Make sure we actually found a token
      if (p == in_begin) {
        // Single character that we didn't handle above
        out_begin = in_begin;
        out_end = in_begin + 1;
        paletteIndex = TextEditor::PaletteIndex::String;
        return true;
      }

      // If the token is followed by a colon + whitespace, it's a YAML key
      if (p < in_end && *p == ':' &&
          (p + 1 >= in_end || *(p + 1) == ' ' || *(p + 1) == '\t' || *(p + 1) == '\n' ||
           *(p + 1) == '\r')) {
        out_begin = in_begin;
        out_end = p;
        paletteIndex = TextEditor::PaletteIndex::Default;
        return true;
      }

      // Handle standalone colon (YAML key separator)
      if (in_begin < in_end && *in_begin == ':') {
        out_begin = in_begin;
        out_end = in_begin + 1;
        paletteIndex = TextEditor::PaletteIndex::Punctuation;
        return true;
      }

      // Check if it's a standalone number
      std::string token(in_begin, p);
      bool isStandaloneNumber = true;

      // Check if the entire token is a valid number
      const char* numCheck = in_begin;

      // Optional sign
      if (numCheck < p && (*numCheck == '+' || *numCheck == '-')) {
        numCheck++;
      }

      // Must have at least one digit
      if (numCheck >= p || *numCheck < '0' || *numCheck > '9') {
        isStandaloneNumber = false;
      } else {
        // Parse integer part
        while (numCheck < p && *numCheck >= '0' && *numCheck <= '9') {
          numCheck++;
        }

        // Optional decimal part
        if (numCheck < p && *numCheck == '.') {
          numCheck++;
          while (numCheck < p && *numCheck >= '0' && *numCheck <= '9') {
            numCheck++;
          }
        }

        // Optional scientific notation
        if (numCheck < p && (*numCheck == 'e' || *numCheck == 'E')) {
          numCheck++;
          if (numCheck < p && (*numCheck == '+' || *numCheck == '-')) {
            numCheck++;
          }
          while (numCheck < p && *numCheck >= '0' && *numCheck <= '9') {
            numCheck++;
          }
        }

        // If we didn't consume the entire token, it's not a pure number
        if (numCheck != p) {
          isStandaloneNumber = false;
        }
      }

      if (isStandaloneNumber) {
        out_begin = in_begin;
        out_end = p;
        paletteIndex = TextEditor::PaletteIndex::Number;
        return true;
      }

      // Check if it's a keyword
      if (langDef.mKeywords.count(token) > 0) {
        out_begin = in_begin;
        out_end = p;
        paletteIndex = TextEditor::PaletteIndex::Keyword;
        return true;
      }

      // Everything else is a string
      out_begin = in_begin;
      out_end = p;
      paletteIndex = TextEditor::PaletteIndex::KnownIdentifier;
      return true;
    };

    langDef.mCommentStart = "#";
    langDef.mCommentEnd = "";
    langDef.mSingleLineComment = "#";

    langDef.mCaseSensitive = true;
    langDef.mAutoIndentation = true;

    langDef.mName = "YAML";

    inited = true;
  }
  return langDef;
}

void TextEditorWidget::SetConfig(const Config& cfg) {
  m_Config = cfg;
  for (const auto& item : TOOL_BAR()) {
    auto contained = cfg.ToolBarItems.contains(item.first);
    m_BtnsToolBar->SetVisible(item.first, contained);
  }
}

TextEditorWidget::Config& TextEditorWidget::GetConfig() {
  return m_Config;
}

void TextEditorWidget::SetFilePath(const std::filesystem::path& path) {
  m_FilePath = path;
}

const std::filesystem::path& TextEditorWidget::GetFilePath() const {
  return m_FilePath;
}

void TextEditorWidget::SetText(const std::string& msg) {
  std::lock_guard<std::mutex> lock(m_EditorMutex);
  m_Editor.SetText(msg);
  m_TextDirty = false;
}

void TextEditorWidget::SetFilterOutFirstYMLComment(bool enableFiltering) {
  m_FilterOutFirstYMLComment = enableFiltering;
}

void TextEditorWidget::AppendText(const std::string& msg, bool addNewLine) {
  {
    std::lock_guard<std::mutex> lock(m_EditorMutex);
    std::string currentText = m_Editor.GetText();
    currentText += msg;
    if (addNewLine) {
      currentText += "\n";
    }
    m_TextDirty = false;
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
  auto text = m_Editor.GetText();
  // For some reason, the GetText function of the third party editor
  // adds an additional blank line when text is retrieved
  // Here we work around that
  if (text.ends_with("\n\n")) {
    text.resize(text.size() - 1);
  }
  return text;
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
  outFile << m_HeaderCommentBlock << GetText();
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

  std::istringstream contentStream(content);
  std::vector<std::string> lines;
  std::string line;

  while (std::getline(contentStream, line)) {
    lines.push_back(line);
  }

  size_t contentStart = 0;
  if (m_FilterOutFirstYMLComment) {
    bool foundComment = false;
    bool foundBlankPostComment = false;
    for (size_t i = 0; i < lines.size(); ++i) {
      if (IsCommentLine(lines[i])) {
        if (foundComment && foundBlankPostComment) {
          contentStart = i;
          break;
        }
        foundComment = true;
      } else if (IsBlankLine(lines[i])) {
        foundBlankPostComment = true;
      } else {
        contentStart = i;
        break;
      }
    }
    std::stringstream headerStream;
    for (size_t i = 0; i < contentStart; ++i) {
      headerStream << lines[i] << "\n";
    }
    m_HeaderCommentBlock = headerStream.str();
  }

  std::stringstream displayStream;
  for (size_t i = contentStart; i < lines.size(); ++i) {
    displayStream << lines[i];
    if (i < lines.size() - 1) {
      displayStream << "\n";
    }
  }

  m_Editor.SetText(displayStream.str());
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
  if (m_ShowCheckStatus) {
    std::string validateMessage = m_LastCheckErrorMessage.has_value()
                                      ? m_LastCheckErrorMessage.value()
                                      : "No issues detected";
    ImGui::Text("Check Status: ");
    ImGui::SameLine();
    ImGui::Text(validateMessage.c_str());
  }
  m_Editor.Render(m_Name.c_str());
}

void TextEditorWidget::SetSendByEmailCallback(std::function<void(const std::string&)> callback) {
  m_OnSendByEmail = std::move(callback);
}

void TextEditorWidget::SetLiveCheck(bool doLiveCheck) {
  m_DoLiveCheck = doLiveCheck;
}

void TextEditorWidget::SetShowCheckStatus(bool showCheckStatus) {
  m_ShowCheckStatus = showCheckStatus;
}

void TextEditorWidget::RenderToolbar(ImVec2 size) {
  auto width = m_BtnsToolBar->GetSize().x;
  if ((ImGui::GetCursorPosX() + width + ImGui::GetStyle().WindowPadding.x) > size.x) {
    ImGui::NewLine();
  } else {
    ImGui::SetCursorPosX(size.x - width - ImGui::GetStyle().WindowPadding.x);
  }

  bool changedContent = m_Editor.IsTextChanged();
  if (!m_OnSave) {
    changedContent &= !m_FilePath.empty();
  }
  m_TextDirty |= changedContent && m_Editor.CanUndo();

  if (m_DoLiveCheck && m_TextDirty && changedContent) {
    RunCheck();
  }

  m_BtnsToolBar->SetEnabled(TOOL_BAR_ITEMS::SAVE, m_TextDirty);
  m_BtnsToolBar->SetEnabled(TOOL_BAR_ITEMS::REVERT, m_TextDirty);
  m_BtnsToolBar->SetEnabled(TOOL_BAR_ITEMS::UNDO, m_Editor.CanUndo());
  m_BtnsToolBar->SetEnabled(TOOL_BAR_ITEMS::REDO, m_Editor.CanRedo());
  if (m_LastCheckResult.has_value()) {
    m_BtnsToolBar->SetStatus(TOOL_BAR_ITEMS::CHECK, m_LastCheckResult.value()
                                                        ? ButtonStatus::Success
                                                        : ButtonStatus::Failure);
    if (m_TextDirty && !m_DoLiveCheck) {
      m_LastCheckResult = std::nullopt;
    }
  } else {
    m_BtnsToolBar->SetStatus(TOOL_BAR_ITEMS::CHECK, ButtonStatus::Default);
  }

  if (m_BtnsToolBar->Render(true)) {
    switch (m_BtnsToolBar->Selected()) {
    case TOOL_BAR_ITEMS::SAVE:
      if (m_OnSave) {
        m_TextDirty = !m_OnSave(m_FilePath);
      } else {
        if (!m_FilePath.empty()) {
          m_TextDirty = !SaveToFile(m_FilePath);
        } else {
          LOG_WARNING << "Save button clicked but no file path is set.";
        }
      }
      break;
    case TOOL_BAR_ITEMS::REVERT:
      if (m_OnRevert) {
        m_TextDirty = !m_OnRevert(m_FilePath);
      } else {
        if (!m_FilePath.empty()) {
          m_TextDirty = !LoadFromFile(m_FilePath);
        } else {
          LOG_WARNING << "Revert button clicked but no file path is set.";
        }
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
      RunCheck();
      break;
    case TOOL_BAR_ITEMS::SEND_BY_EMAIL: {
      if (m_OnSendByEmail) {
        m_OnSendByEmail(m_Editor.GetText());
      }
      break;
    case TOOL_BAR_ITEMS::EXPORT:
      if (m_OnExport) {
        m_OnExport(m_FilePath);
      }
    default:
      break;
    }
    }
  }
}

void TextEditorWidget::RunCheck() {
  if (m_OnCheck) {
    auto result = m_OnCheck(m_Editor.GetText());
    m_LastCheckErrorMessage = result;
    TextEditor::Breakpoints breakpoints;
    if (result.has_value()) {
      std::string msg = result.value();
      LOG_ERROR << "Live check detected issues: " << msg;
      // if msg contains a 'line' followed by a number, highlight it in the editor (this is a very naive implementation and can be improved)
      if (auto pos = msg.find("line"); pos != std::string::npos) {
        size_t lineStart = pos + 4; // length of "line"
        while (lineStart < msg.size() && (msg[lineStart] < '0' || msg[lineStart] > '9')) {
          lineStart++;
        }
        size_t lineEnd = lineStart;
        while (lineEnd < msg.size() && msg[lineEnd] >= '0' && msg[lineEnd] <= '9') {
          lineEnd++;
        }
        if (lineStart < msg.size() && lineEnd > lineStart) {
          int lineNumber = std::stoi(msg.substr(lineStart, lineEnd - lineStart));
          breakpoints.insert(lineNumber);
        }
      }
    }
    m_Editor.SetBreakpoints(breakpoints);
    m_LastCheckResult = !result.has_value();
  }
}
} // namespace ImGuiHelper
} // namespace gits
