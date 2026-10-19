// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

class TextEditor;

namespace gits::ImGuiHelper {

enum class TextMatchMode {
  Literal,
  // Each '#' matches a literal '#' followed by digits; whitespace before it is optional.
  NumberedWildcard
};

struct TextStructureRule {
  int Level = 1;
  std::vector<std::string> Keywords;
  std::string Label;
  TextMatchMode MatchMode = TextMatchMode::Literal;
  // Label and close the pending group of preceding children at this level.
  bool EndsGroup = false;
};

struct TextNodeInfo {
  // Zero-based, inclusive source line range.
  int StartLine = -1;
  int EndLine = -1;
  // The keyword's zero-based line, which may be at the end of the group.
  int MatchedLine = -1;
  int Level = 0;
  std::string Path;
  std::string ParentPath;
  std::vector<std::string> Children;
  std::string BaseLabel;
  std::string DisplayName;
  bool IsPlaceholder = false;
};

class TextTreeViewer {
public:
  void SetTextEditor(TextEditor* editor);
  // The caller keeps the supplied editor synchronized with this file.
  bool LoadFile(const std::filesystem::path& path, const std::vector<TextStructureRule>& rules);
  void Render();

  TextNodeInfo GetSelectedNodeInfo() const;
  std::string GetSelectedNodePath() const;
  void SelectNode(const std::string& nodePath);

  std::string GetPathFromLineNumber(int lineNumber) const;
  bool IsLoaded() const;
  std::string GetError() const;

private:
  void AddMatchedNode(const TextStructureRule& rule,
                      int lineNumber,
                      std::vector<std::string>& ancestors,
                      std::unordered_map<std::string, size_t>& labelCounts);
  void RenderNode(const std::string& nodePath);
  void ScrollToNode(const std::string& nodePath);

  std::unordered_map<std::string, TextNodeInfo> m_NodeInfoMap;
  std::vector<std::string> m_RootPaths;
  TextEditor* m_TextEditor = nullptr;
  std::string m_SelectedNodePath;
  bool m_IsLoaded = false;
  std::string m_Error;
};
} // namespace gits::ImGuiHelper
