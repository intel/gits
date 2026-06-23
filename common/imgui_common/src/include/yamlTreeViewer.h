// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <yaml-cpp/yaml.h>
#include "imgui.h"
#include <TextEditor.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace gits::ImGuiHelper {

struct YamlNodeInfo {
  int StartLine = -1;
  int EndLine = -1;
  std::string Path;
  YAML::NodeType::value Type;
  bool IsLeaf = false;
  std::string DisplayName;
  size_t ChildCount = 0;
  bool HasOnlyLeafs = false;
};

class YamlTreeViewer {
public:
  void SetTextEditor(TextEditor* editor);
  void SetYAMLText(const std::string& yamlString);
  void Render();

  YamlNodeInfo GetSelectedNodeInfo() const;
  std::string GetSelectedNodePath() const;
  void SelectNode(const std::string& nodePath);

  const std::string GetPathFromLineNumber(int lineNumber) const;

  bool IsValidYAML() const;

  std::string GetParseError() const;

private:
  void ProcessYAML(const std::string& yamlString);
  void BuildNodeInfoWithMarks(const YAML::Node& node,
                              const std::string& key,
                              const std::string& parentPath,
                              const std::vector<std::string>& lines);
  void BuildNodeInfoEstimated(const YAML::Node& node,
                              const std::string& key,
                              const std::string& parentPath,
                              const std::vector<std::string>& lines,
                              int estimatedLine);
  void BuildLineMappingFromMarks(const std::string& yamlString);
  int FindKeyLineInText(const std::string& key,
                        const std::vector<std::string>& lines,
                        int startFrom);
  std::string TrimString(const std::string& str);

  std::string NodeTypeToString(YAML::NodeType::value type) const;

  void RenderRootChildren(const YAML::Node& rootNode);
  void RenderNode(const YAML::Node& node, const std::string& key, const std::string& parentPath);
  void RenderMapNode(const YAML::Node& node,
                     const std::string& key,
                     const std::string& currentPath);
  void RenderSequenceNode(const YAML::Node& node,
                          const std::string& key,
                          const std::string& currentPath);
  void ScrollToNode(const std::string& nodePath);

  std::unordered_map<std::string, YamlNodeInfo> m_NodeInfoMap;
  TextEditor* m_TextEditor = nullptr;
  std::string m_SelectedNodePath;
  YAML::Node m_RootNode;
  bool m_HasValidYaml = false;
  std::string m_ParseError;
  bool m_ShouldSelect = false;
};
} // namespace gits::ImGuiHelper
