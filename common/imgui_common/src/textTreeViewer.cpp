// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "textTreeViewer.h"
#include "imgui.h"
#include <TextEditor.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
#include <unordered_map>
#include <utility>

#if 0

namespace gits::ImGuiHelper {
void TextTreeViewer::SetTextEditor(TextEditor* editor) {
  m_TextEditor = editor;
}

void TextTreeViewer::SetYAMLText(const std::string& yamlString) {
  ProcessYAML(yamlString);
}

void TextTreeViewer::Render() {
  if (!m_HasValidYaml) {
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "YAML Parse Error:");
    ImGui::TextWrapped("%s", m_ParseError.c_str());
    return;
  }

  if (m_RootNode.IsNull()) {
    ImGui::Text("Empty YAML document");
    return;
  }

  RenderRootChildren(m_RootNode);
}

YamlNodeInfo TextTreeViewer::GetSelectedNodeInfo() const {
  auto it = m_NodeInfoMap.find(m_SelectedNodePath);
  if (it != m_NodeInfoMap.end()) {
    return it->second;
  }
  return YamlNodeInfo{};
}

std::string TextTreeViewer::GetSelectedNodePath() const {
  return m_SelectedNodePath;
}

void TextTreeViewer::SelectNode(const std::string& nodePath) {
  if (m_NodeInfoMap.find(nodePath) != m_NodeInfoMap.end()) {
    m_SelectedNodePath = nodePath;
    ScrollToNode(nodePath);
  }
}

const std::string TextTreeViewer::GetPathFromLineNumber(int lineNumber) const {
  for (const auto& pair : m_NodeInfoMap) {
    const YamlNodeInfo& info = pair.second;
    if (lineNumber >= info.StartLine && lineNumber <= info.EndLine) {
      return pair.first;
    }
  }
  return "";
}

bool TextTreeViewer::IsValidYAML() const {
  return m_HasValidYaml;
}

std::string TextTreeViewer::GetParseError() const {
  return m_ParseError;
}

void TextTreeViewer::ProcessYAML(const std::string& yamlString) {
  try {
    m_NodeInfoMap.clear();
    m_SelectedNodePath.clear();
    m_HasValidYaml = false;
    m_ParseError.clear();

    if (yamlString.empty()) {
      m_HasValidYaml = true;
      m_RootNode = YAML::Node();
      return;
    }

    // Parse YAML with detailed parsing to get marks
    m_RootNode = YAML::Load(yamlString);

    // Build line mapping using a different approach
    BuildLineMappingFromMarks(yamlString);

    m_HasValidYaml = true;

  } catch (const YAML::Exception& e) {
    m_HasValidYaml = false;
    m_ParseError = e.what();
    m_RootNode = YAML::Node();
  }
}

void TextTreeViewer::BuildLineMappingFromMarks(const std::string& yamlString) {
  if (yamlString.empty()) {
    return;
  }

  // Split into lines for reference
  std::vector<std::string> lines;
  std::istringstream stream(yamlString);
  std::string line;
  while (std::getline(stream, line)) {
    lines.push_back(line);
  }

  // Use the Mark information from nodes when available
  try {
    // Parse with detailed node information
    YAML::Node detailedRoot = YAML::Load(yamlString);
    BuildNodeInfoWithMarks(detailedRoot, "root", "", lines);
  } catch (...) {
    // Fallback to simple line estimation
    BuildNodeInfoEstimated(m_RootNode, "root", "", lines, 0);
  }
}

void TextTreeViewer::BuildNodeInfoWithMarks(const YAML::Node& node,
                                            const std::string& key,
                                            const std::string& parentPath,
                                            const std::vector<std::string>& lines) {

  std::string currentPath;
  if (key == "root") {
    currentPath = "root";
  } else if (parentPath == "root") {
    currentPath = key;
  } else if (parentPath.empty()) {
    currentPath = key;
  } else {
    currentPath = parentPath + "." + key;
  }

  YamlNodeInfo info;
  info.Path = currentPath;
  info.Type = node.Type();
  info.DisplayName = key;

  // Try to get line information from Mark
  int startLine = -1;
  if (node.Mark().line != -1) {
    startLine = static_cast<int>(node.Mark().line);
  } else {
    // Fallback: search for the key in text
    startLine = FindKeyLineInText(key, lines, 0);
  }

  info.StartLine = startLine;
  int endLine = startLine;

  switch (node.Type()) {
  case YAML::NodeType::Map: {
    info.IsLeaf = false;
    info.ChildCount = node.size();

    // Check if this map has any non-leaf children (only care about Maps)
    bool hasNonLeafChildren = false;

    for (const auto& pair : node) {
      std::string childKey = pair.first.as<std::string>();
      BuildNodeInfoWithMarks(pair.second, childKey, currentPath, lines);

      // Check if this child is a Map (non-leaf that we care about)
      auto childIt = m_NodeInfoMap.find(currentPath + "." + childKey);
      if (childIt != m_NodeInfoMap.end()) {
        if (childIt->second.Type == YAML::NodeType::Map) {
          hasNonLeafChildren = true;
        }

        // Update end line based on children
        if (childIt->second.EndLine > endLine) {
          endLine = childIt->second.EndLine;
        }
      }
    }

    info.HasOnlyLeafs = !hasNonLeafChildren;
    break;
  }
  case YAML::NodeType::Sequence: {
    info.IsLeaf = false;
    info.ChildCount = node.size();
    info.HasOnlyLeafs = true; // We don't care about sequence children, treat as having only leafs

    for (size_t i = 0; i < node.size(); ++i) {
      std::string indexKey = "[" + std::to_string(i) + "]";
      BuildNodeInfoWithMarks(node[i], indexKey, currentPath, lines);

      // Update end line based on children
      auto childIt = m_NodeInfoMap.find(currentPath + "." + indexKey);
      if (childIt != m_NodeInfoMap.end() && childIt->second.EndLine > endLine) {
        endLine = childIt->second.EndLine;
      }
    }
    break;
  }
  default: {
    info.IsLeaf = true;
    info.ChildCount = 0;
    info.HasOnlyLeafs = false; // Leaf nodes don't have children
    endLine = startLine;
    break;
  }
  }

  info.EndLine = endLine;
  m_NodeInfoMap[currentPath] = info;
}

void TextTreeViewer::BuildNodeInfoEstimated(const YAML::Node& node,
                                            const std::string& key,
                                            const std::string& parentPath,
                                            const std::vector<std::string>& lines,
                                            int estimatedLine) {

  std::string currentPath;
  if (key == "root") {
    currentPath = "root";
  } else if (parentPath == "root") {
    currentPath = key;
  } else if (parentPath.empty()) {
    currentPath = key;
  } else {
    currentPath = parentPath + "." + key;
  }

  YamlNodeInfo info;
  info.Path = currentPath;
  info.Type = node.Type();
  info.DisplayName = key;

  // Estimate line numbers by searching in text
  int startLine = FindKeyLineInText(key, lines, estimatedLine);
  info.StartLine = startLine;

  int currentEstimate = startLine + 1;

  switch (node.Type()) {
  case YAML::NodeType::Map: {
    info.IsLeaf = false;
    info.ChildCount = node.size();

    // Check if this map has any Map children (only care about Maps)
    bool hasNonLeafChildren = false;

    for (const auto& pair : node) {
      std::string childKey = pair.first.as<std::string>();
      BuildNodeInfoEstimated(pair.second, childKey, currentPath, lines, currentEstimate);

      // Check if this child is a Map
      auto childIt = m_NodeInfoMap.find(currentPath + "." + childKey);
      if (childIt != m_NodeInfoMap.end() && childIt->second.Type == YAML::NodeType::Map) {
        hasNonLeafChildren = true;
      }

      currentEstimate++;
    }

    info.HasOnlyLeafs = !hasNonLeafChildren;
    info.EndLine = currentEstimate - 1;
    break;
  }
  case YAML::NodeType::Sequence: {
    info.IsLeaf = false;
    info.ChildCount = node.size();
    info.HasOnlyLeafs = true; // We don't care about sequence children

    for (size_t i = 0; i < node.size(); ++i) {
      std::string indexKey = "[" + std::to_string(i) + "]";
      BuildNodeInfoEstimated(node[i], indexKey, currentPath, lines, currentEstimate);
      currentEstimate++;
    }

    info.EndLine = currentEstimate - 1;
    break;
  }
  default: {
    info.IsLeaf = true;
    info.ChildCount = 0;
    info.HasOnlyLeafs = false; // Leaf nodes don't have children
    info.EndLine = startLine;
    break;
  }
  }

  m_NodeInfoMap[currentPath] = info;
}

int TextTreeViewer::FindKeyLineInText(const std::string& key,
                                      const std::vector<std::string>& lines,
                                      int startFrom) {
  if (key == "root") {
    return 0;
  }

  // Handle array indices
  if (key.length() > 2 && key.front() == '[' && key.back() == ']') {
    for (int i = startFrom; i < static_cast<int>(lines.size()); ++i) {
      std::string trimmed = TrimString(lines[i]);
      if (!trimmed.empty() && trimmed[0] == '-') {
        return i;
      }
    }
    return startFrom;
  }

  // Search for key
  for (int i = startFrom; i < static_cast<int>(lines.size()); ++i) {
    std::string line = lines[i];
    std::string trimmed = TrimString(line);

    if (trimmed.empty() || trimmed[0] == '#') {
      continue;
    }

    size_t colonPos = trimmed.find(':');
    if (colonPos != std::string::npos) {
      std::string lineKey = TrimString(trimmed.substr(0, colonPos));
      if (lineKey == key) {
        return i;
      }
    }
  }

  return startFrom;
}

std::string TextTreeViewer::TrimString(const std::string& str) {
  size_t start = str.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    return "";
  }

  size_t end = str.find_last_not_of(" \t\r\n");
  return str.substr(start, end - start + 1);
}

std::string TextTreeViewer::NodeTypeToString(YAML::NodeType::value type) const {
  switch (type) {
  case YAML::NodeType::Null:
    return "Null";
  case YAML::NodeType::Scalar:
    return "Scalar";
  case YAML::NodeType::Sequence:
    return "Sequence";
  case YAML::NodeType::Map:
    return "Map";
  case YAML::NodeType::Undefined:
    return "Undefined";
  default:
    return "Unknown";
  }
}

void TextTreeViewer::RenderRootChildren(const YAML::Node& rootNode) {
  switch (rootNode.Type()) {
  case YAML::NodeType::Map: {
    for (const auto& pair : rootNode) {
      std::string key = pair.first.as<std::string>();
      RenderNode(pair.second, key, "");
    }
    break;
  }
  case YAML::NodeType::Sequence: {
    for (size_t i = 0; i < rootNode.size(); ++i) {
      std::string indexKey = "[" + std::to_string(i) + "]";
      RenderNode(rootNode[i], indexKey, "");
    }
    break;
  }
  case YAML::NodeType::Scalar:
  case YAML::NodeType::Null: {
    std::string value = rootNode.as<std::string>();
    ImGui::Text("Value: %s", value.c_str());
    break;
  }
  default:
    ImGui::Text("Unknown YAML structure");
    break;
  }
}

void TextTreeViewer::RenderNode(const YAML::Node& node,
                                const std::string& key,
                                const std::string& parentPath) {
  std::string currentPath;
  if (parentPath.empty()) {
    currentPath = key;
  } else {
    currentPath = parentPath + "." + key;
  }

  switch (node.Type()) {
  case YAML::NodeType::Map:
    RenderMapNode(node, key, currentPath);
    break;
  default:
    break;
  }
}

void TextTreeViewer::RenderMapNode(const YAML::Node& node,
                                   const std::string& key,
                                   const std::string& currentPath) {
  std::string label = key;

  ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnDoubleClick;

  // Get node info to check if it has non-leaf children
  auto it = m_NodeInfoMap.find(currentPath);
  bool hasNonLeafChildren = false;
  if (it != m_NodeInfoMap.end()) {
    hasNonLeafChildren = !it->second.HasOnlyLeafs;
  }

  // Only add arrow if there are non-leaf children to show
  if (hasNonLeafChildren) {
    flags |= ImGuiTreeNodeFlags_OpenOnArrow;
  } else {
    flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
  }

  if (currentPath == m_SelectedNodePath) {
    flags |= ImGuiTreeNodeFlags_Selected;
  }

  bool nodeOpen = ImGui::TreeNodeEx((label + "##" + currentPath).c_str(), flags);

  if (ImGui::IsItemClicked()) {
    m_SelectedNodePath = currentPath;
    ScrollToNode(currentPath);
  }

  // Only render children if we have non-leaf children and the node is open
  if (nodeOpen && hasNonLeafChildren) {
    for (const auto& pair : node) {
      std::string mapKey = pair.first.as<std::string>();
      RenderNode(pair.second, mapKey, currentPath);
    }
    ImGui::TreePop();
  }
}

void TextTreeViewer::RenderSequenceNode(const YAML::Node& node,
                                        const std::string& key,
                                        const std::string& currentPath) {
  std::string label = key + " [" + std::to_string(node.size()) + "]";

  ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnDoubleClick;

  // Get node info to check if it has non-leaf children
  auto it = m_NodeInfoMap.find(currentPath);
  bool hasNonLeafChildren = false;
  if (it != m_NodeInfoMap.end()) {
    hasNonLeafChildren = !it->second.HasOnlyLeafs;
  }

  // Only add arrow if there are non-leaf children to show
  if (hasNonLeafChildren) {
    flags |= ImGuiTreeNodeFlags_OpenOnArrow;
  } else {
    flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
  }

  if (currentPath == m_SelectedNodePath) {
    flags |= ImGuiTreeNodeFlags_Selected;
  }

  bool nodeOpen = ImGui::TreeNodeEx((label + "##" + currentPath).c_str(), flags);

  if (ImGui::IsItemClicked()) {
    m_SelectedNodePath = currentPath;
    ScrollToNode(currentPath);
  }

  // Only render children if we have non-leaf children and the node is open
  if (nodeOpen && hasNonLeafChildren) {
    for (size_t i = 0; i < node.size(); ++i) {
      std::string indexKey = "[" + std::to_string(i) + "]";
      RenderNode(node[i], indexKey, currentPath);
    }
    ImGui::TreePop();
  }
}

void TextTreeViewer::ScrollToNode(const std::string& nodePath) {
  if (!m_TextEditor) {
    return;
  }

  auto it = m_NodeInfoMap.find(nodePath);
  if (it != m_NodeInfoMap.end()) {
    const YamlNodeInfo& info = it->second;
    if (info.StartLine >= 0) {
      TextEditor::Coordinates coords;
      coords.mLine = std::max(0, info.StartLine - 1);
      coords.mColumn = 0;
      m_TextEditor->SetCursorPosition(coords);

      if (m_ShouldSelect) {
        if (info.EndLine > info.StartLine) {
          TextEditor::Coordinates endCoords;
          endCoords.mLine = info.EndLine;
          endCoords.mColumn = 0;
          m_TextEditor->SetSelection(coords, endCoords);
        }
      }
    }
  }
}

} // namespace gits::ImGuiHelper
#endif

namespace {
std::string FoldAscii(std::string text) {
  for (char& c : text) {
    if (c >= 'A' && c <= 'Z') {
      c = static_cast<char>(c + ('a' - 'A'));
    }
  }
  return text;
}

size_t FindNumberedKeyword(const std::string& text, const std::string& keyword) {
  if (keyword.find('#') == std::string::npos) {
    return text.find(keyword);
  }

  for (size_t start = 0; start < text.size(); ++start) {
    size_t textPosition = start;
    size_t keywordPosition = 0;
    while (keywordPosition < keyword.size()) {
      size_t marker = keyword.find('#', keywordPosition);
      size_t literalEnd = marker == std::string::npos ? keyword.size() : marker;
      if (marker != std::string::npos) {
        while (literalEnd > keywordPosition &&
               (keyword[literalEnd - 1] == ' ' || keyword[literalEnd - 1] == '\t')) {
          --literalEnd;
        }
      }
      size_t literalLength = literalEnd - keywordPosition;
      if (text.compare(textPosition, literalLength, keyword, keywordPosition, literalLength) != 0) {
        break;
      }
      textPosition += literalLength;
      if (marker == std::string::npos) {
        return start;
      }

      while (textPosition < text.size() &&
             (text[textPosition] == ' ' || text[textPosition] == '\t')) {
        ++textPosition;
      }
      if (textPosition == text.size() || text[textPosition] != '#') {
        break;
      }
      ++textPosition;
      size_t numberStart = textPosition;
      while (textPosition < text.size() && text[textPosition] >= '0' && text[textPosition] <= '9') {
        ++textPosition;
      }
      if (textPosition == numberStart) {
        break;
      }
      keywordPosition = marker + 1;
      if (keywordPosition == keyword.size()) {
        return start;
      }
    }
  }
  return std::string::npos;
}
} // namespace

namespace gits::ImGuiHelper {
void TextTreeViewer::SetTextEditor(TextEditor* editor) {
  m_TextEditor = editor;
}

bool TextTreeViewer::LoadFile(const std::filesystem::path& path,
                              const std::vector<TextStructureRule>& rules) {
  m_NodeInfoMap.clear();
  m_RootPaths.clear();
  m_SelectedNodePath.clear();
  m_IsLoaded = false;
  m_Error.clear();

  auto foldedRules = rules;
  for (size_t i = 0; i < foldedRules.size(); ++i) {
    auto& rule = foldedRules[i];
    if (rule.Level <= 0 || rule.Keywords.empty()) {
      m_Error = "Rule " + std::to_string(i + 1) +
                " requires a positive level and at least one nonempty keyword.";
      return false;
    }
    for (auto& keyword : rule.Keywords) {
      if (keyword.empty()) {
        m_Error = "Rule " + std::to_string(i + 1) + " contains an empty keyword.";
        return false;
      }
      keyword = FoldAscii(std::move(keyword));
    }
  }

  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) {
    m_Error = "Unable to open text file.";
    return false;
  }

  std::vector<std::string> ancestors;
  std::unordered_map<std::string, size_t> labelCounts;
  std::string line;
  int lineNumber = 0;
  while (std::getline(file, line)) {
    if (lineNumber == (std::numeric_limits<int>::max)()) {
      m_Error = "Text file exceeds the editor's supported line count.";
      break;
    }
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    line = FoldAscii(std::move(line));

    const TextStructureRule* winner = nullptr;
    size_t winnerPosition = std::string::npos;
    for (const auto& rule : foldedRules) {
      size_t position = std::string::npos;
      for (const auto& keyword : rule.Keywords) {
        size_t keywordPosition = rule.MatchMode == TextMatchMode::NumberedWildcard
                                     ? FindNumberedKeyword(line, keyword)
                                     : line.find(keyword);
        position = (std::min)(position, keywordPosition);
      }
      if (position != std::string::npos &&
          (!winner || rule.Level < winner->Level ||
           (rule.Level == winner->Level && position < winnerPosition))) {
        winner = &rule;
        winnerPosition = position;
      }
    }

    if (winner) {
      AddMatchedNode(*winner, lineNumber, ancestors, labelCounts);
    }
    ++lineNumber;
  }

  if (file.bad() || (file.fail() && !file.eof())) {
    m_Error = "Unable to read text file completely.";
  }
  if (!m_Error.empty()) {
    m_NodeInfoMap.clear();
    m_RootPaths.clear();
    return false;
  }

  for (const auto& nodePath : ancestors) {
    m_NodeInfoMap.at(nodePath).EndLine = lineNumber - 1;
  }

  m_IsLoaded = true;
  return true;
}

void TextTreeViewer::AddMatchedNode(const TextStructureRule& rule,
                                    int lineNumber,
                                    std::vector<std::string>& ancestors,
                                    std::unordered_map<std::string, size_t>& labelCounts) {
  if (rule.EndsGroup && ancestors.size() >= static_cast<size_t>(rule.Level) &&
      m_NodeInfoMap.at(ancestors[rule.Level - 1]).IsPlaceholder) {
    while (ancestors.size() > static_cast<size_t>(rule.Level)) {
      m_NodeInfoMap.at(ancestors.back()).EndLine = lineNumber - 1;
      ancestors.pop_back();
    }
    auto& info = m_NodeInfoMap.at(ancestors.back());
    info.IsPlaceholder = false;
    info.MatchedLine = lineNumber;
    info.EndLine = lineNumber;
    info.BaseLabel = rule.Label;
    info.DisplayName = rule.Label + " " + std::to_string(++labelCounts[rule.Label]);
    ancestors.pop_back();
    return;
  }

  while (ancestors.size() >= static_cast<size_t>(rule.Level)) {
    m_NodeInfoMap.at(ancestors.back()).EndLine = lineNumber - 1;
    ancestors.pop_back();
  }

  while (ancestors.size() < static_cast<size_t>(rule.Level)) {
    TextNodeInfo info;
    info.Level = static_cast<int>(ancestors.size() + 1);
    info.StartLine = lineNumber;
    info.EndLine = lineNumber;
    info.Path = "node-" + std::to_string(m_NodeInfoMap.size());
    info.IsPlaceholder = info.Level < rule.Level;
    if (info.IsPlaceholder) {
      info.DisplayName = "Level " + std::to_string(info.Level) + " (missing)";
    } else {
      info.MatchedLine = lineNumber;
      info.BaseLabel = rule.Label;
      info.DisplayName = rule.Label + " " + std::to_string(++labelCounts[rule.Label]);
    }

    if (ancestors.empty()) {
      m_RootPaths.push_back(info.Path);
    } else {
      info.ParentPath = ancestors.back();
      m_NodeInfoMap.at(info.ParentPath).Children.push_back(info.Path);
    }
    ancestors.push_back(info.Path);
    m_NodeInfoMap.emplace(ancestors.back(), std::move(info));
  }
  if (rule.EndsGroup) {
    ancestors.pop_back();
  }
}

bool TextTreeViewer::IsLoaded() const {
  return m_IsLoaded;
}

std::string TextTreeViewer::GetError() const {
  return m_Error;
}

TextNodeInfo TextTreeViewer::GetSelectedNodeInfo() const {
  auto it = m_NodeInfoMap.find(m_SelectedNodePath);
  return it != m_NodeInfoMap.end() ? it->second : TextNodeInfo{};
}

std::string TextTreeViewer::GetSelectedNodePath() const {
  return m_SelectedNodePath;
}

void TextTreeViewer::SelectNode(const std::string& nodePath) {
  if (m_NodeInfoMap.find(nodePath) != m_NodeInfoMap.end()) {
    m_SelectedNodePath = nodePath;
    ScrollToNode(nodePath);
  }
}

std::string TextTreeViewer::GetPathFromLineNumber(int lineNumber) const {
  const TextNodeInfo* deepest = nullptr;
  for (const auto& pair : m_NodeInfoMap) {
    const auto& info = pair.second;
    if (!info.IsPlaceholder && lineNumber >= info.StartLine && lineNumber <= info.EndLine &&
        (!deepest || info.Level > deepest->Level)) {
      deepest = &info;
    }
  }
  return deepest ? deepest->Path : std::string{};
}

void TextTreeViewer::Render() {
  if (!m_Error.empty()) {
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Text File Error:");
    ImGui::TextWrapped("%s", m_Error.c_str());
    return;
  }
  if (m_RootPaths.empty()) {
    ImGui::TextUnformatted(m_IsLoaded ? "No matching lines" : "No text file loaded");
    return;
  }

  ImGui::PushID(this);
  for (const auto& nodePath : m_RootPaths) {
    RenderNode(nodePath);
  }
  ImGui::PopID();
}

void TextTreeViewer::RenderNode(const std::string& nodePath) {
  const auto& info = m_NodeInfoMap.at(nodePath);
  ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnDoubleClick;
  if (info.Children.empty()) {
    flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
  } else {
    flags |= ImGuiTreeNodeFlags_OpenOnArrow;
  }
  if (nodePath == m_SelectedNodePath) {
    flags |= ImGuiTreeNodeFlags_Selected;
  }

  bool open = ImGui::TreeNodeEx(nodePath.c_str(), flags, "%s", info.DisplayName.c_str());
  if (ImGui::IsItemClicked()) {
    SelectNode(nodePath);
  }
  if (open && !info.Children.empty()) {
    for (const auto& child : info.Children) {
      RenderNode(child);
    }
    ImGui::TreePop();
  }
}

void TextTreeViewer::ScrollToNode(const std::string& nodePath) {
  if (!m_TextEditor) {
    return;
  }
  auto it = m_NodeInfoMap.find(nodePath);
  if (it == m_NodeInfoMap.end()) {
    return;
  }
  const TextNodeInfo* info = &it->second;
  while (info->IsPlaceholder && !info->Children.empty()) {
    info = &m_NodeInfoMap.at(info->Children.front());
  }
  if (!info->IsPlaceholder && info->MatchedLine >= 0) {
    TextEditor::Coordinates coords(info->MatchedLine, 0);
    m_TextEditor->SetSelection(coords, coords);
    m_TextEditor->SetCursorPosition(coords);
  }
}
} // namespace gits::ImGuiHelper
