// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "yamlTreeViewer.h"

namespace gits::ImGuiHelper {
void YamlTreeViewer::SetTextEditor(TextEditor* editor) {
  m_TextEditor = editor;
}

void YamlTreeViewer::SetYAMLText(const std::string& yamlString) {
  ProcessYAML(yamlString);
}

void YamlTreeViewer::Render() {
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

YamlNodeInfo YamlTreeViewer::GetSelectedNodeInfo() const {
  auto it = m_NodeInfoMap.find(m_SelectedNodePath);
  if (it != m_NodeInfoMap.end()) {
    return it->second;
  }
  return YamlNodeInfo{};
}

std::string YamlTreeViewer::GetSelectedNodePath() const {
  return m_SelectedNodePath;
}

void YamlTreeViewer::SelectNode(const std::string& nodePath) {
  if (m_NodeInfoMap.find(nodePath) != m_NodeInfoMap.end()) {
    m_SelectedNodePath = nodePath;
    ScrollToNode(nodePath);
  }
}

const std::string YamlTreeViewer::GetPathFromLineNumber(int lineNumber) const {
  for (const auto& pair : m_NodeInfoMap) {
    const YamlNodeInfo& info = pair.second;
    if (lineNumber >= info.StartLine && lineNumber <= info.EndLine) {
      return pair.first;
    }
  }
  return "";
}

bool YamlTreeViewer::IsValidYAML() const {
  return m_HasValidYaml;
}

std::string YamlTreeViewer::GetParseError() const {
  return m_ParseError;
}

void YamlTreeViewer::ProcessYAML(const std::string& yamlString) {
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

void YamlTreeViewer::BuildLineMappingFromMarks(const std::string& yamlString) {
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

void YamlTreeViewer::BuildNodeInfoWithMarks(const YAML::Node& node,
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

void YamlTreeViewer::BuildNodeInfoEstimated(const YAML::Node& node,
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

int YamlTreeViewer::FindKeyLineInText(const std::string& key,
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

std::string YamlTreeViewer::TrimString(const std::string& str) {
  size_t start = str.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    return "";
  }

  size_t end = str.find_last_not_of(" \t\r\n");
  return str.substr(start, end - start + 1);
}

std::string YamlTreeViewer::NodeTypeToString(YAML::NodeType::value type) const {
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

void YamlTreeViewer::RenderRootChildren(const YAML::Node& rootNode) {
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

void YamlTreeViewer::RenderNode(const YAML::Node& node,
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

void YamlTreeViewer::RenderMapNode(const YAML::Node& node,
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

void YamlTreeViewer::RenderSequenceNode(const YAML::Node& node,
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

void YamlTreeViewer::ScrollToNode(const std::string& nodePath) {
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
