// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "contentPanel.h"

#include <map>

#include "imGuiHelper.h"
#include "studioLabels.h"
#include "baseButtonGroup.h"
#include "buttonGroup.h"
#include "studioContext.h"
#include "moduleRegistry.h"
#include "contextHelper.h"

namespace {
using SideBarItem = gits::gui::SideBarItem;
using ItemType = gits::ImGuiHelper::ItemType;

static const auto& SIDE_BAR() {
  static const std::map<SideBarItem, gits::ImGuiHelper::ButtonGroupItem> items = {
      {SideBarItem::LABEL_STREAM,
       {.label = "Stream", .tooltip = "Work with the stream", .type = ItemType::Label}},
      {SideBarItem::METADATA, {"Metadata", "Show the stream metadata"}},
      {SideBarItem::API_TRACE, {"API Trace", "Show api trace"}},
      {SideBarItem::LABEL_LOG, {.label = "Log", .tooltip = "Show logs", .type = ItemType::Label}},
      {SideBarItem::LOG, {"GITS Log", "Show the GITS log"}},
      {SideBarItem::APP_LOG, {"Studio Log", "Show studio log"}}};
  return items;
}
static const auto& SIDE_BAR_VEC() {
  static const std::vector<std::string> values = gits::gui::Labels::extractLabels(SIDE_BAR());
  return values;
}

} // namespace

namespace gits::gui {
ContentPanel::ContentPanel()
    : m_BtnsSideBar(SIDE_BAR(), false, true, true), m_MetaDataPanel(), m_TracePanel() {
  auto& registry = ModuleRegistry::GetInstance();
  registry.CreateModules();

  auto& context = StudioContext::GetInstance();

  if (context.IsLiveMode()) {
    m_BtnsSideBar.SelectEntry(SideBarItem::LOG);
  } else {
    m_BtnsSideBar.SelectEntry(SideBarItem::METADATA);
  }

  EventBus::GetInstance().subscribe<StudioEvent>(
      std::bind(&ContentPanel::StudioEventCallback, this, std::placeholders::_1),
      {StudioEvent::Type::LiveStreamModeChanged});
  EventBus::GetInstance().subscribe<ActionEvent>(
      std::bind(&ContentPanel::ActionCallback, this, std::placeholders::_1));
  EventBus::GetInstance().subscribe<NavigateEvent>(
      std::bind(&ContentPanel::NavigateCallback, this, std::placeholders::_1));
}

ContentPanel::~ContentPanel() {
  auto& registry = ModuleRegistry::GetInstance();
  registry.DestroyModules();
}

void ContentPanel::Render() {

  auto& context = StudioContext::GetInstance();
  const auto widthColumn = 250.0f;
  const auto availableArea = ImGui::GetContentRegionAvail();
  auto& registry = ModuleRegistry::GetInstance();

  if (ImGui::BeginTable("MainLayoutTable", 2, ImGuiTableFlags_BordersInnerV,
                        ImVec2(0.0f, availableArea.y))) {
    ImGui::TableSetupColumn("Sidebar", ImGuiTableColumnFlags_WidthFixed, widthColumn - 12.0f);
    ImGui::TableSetupColumn("Content", ImGuiTableColumnFlags_WidthStretch);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);

    if (registry.RenderModuleButtons()) {
      if (registry.BtnsModules->HasSelection()) {
        m_BtnsSideBar.DeselectEntry();
      }
    }
    ImGui::Separator();
    if (m_BtnsSideBar.Render(true)) {
      if (registry.BtnsModules != nullptr) {
        registry.BtnsModules->DeselectEntry();
      }
    }

    ImGui::TableSetColumnIndex(1);
    bool renderDummy = false;
    auto area = ImGui::GetContentRegionAvail();
    if (m_BtnsSideBar.HasSelection()) {
      switch (m_BtnsSideBar.Selected()) {
      case SideBarItem::METADATA:
        ImGui::BeginChild("StatsArea", ImVec2(area.x, area.y), true);
        m_MetaDataPanel.Render();
        ImGui::EndChild();
        break;
      case SideBarItem::API_TRACE:
        ImGui::BeginChild("TraceArea", ImVec2(area.x, area.y), true);
        m_TracePanel.Render();
        ImGui::EndChild();
        break;
      case SideBarItem::LOG:
        ImGui::BeginChild("gitsLogArea", ImVec2(area.x, area.y), true);
        context.RenderGITSLogEditor();
        ImGui::EndChild();
        break;
      case SideBarItem::APP_LOG:
        ImGui::BeginChild("LauncherLogArea", ImVec2(area.x, area.y), true);
        context.RenderLogEditor();
        ImGui::EndChild();
        break;
      default:
        renderDummy = true;
        break;
      }
    } else if (registry.BtnsModules != nullptr) {
      renderDummy = true;
      if (registry.BtnsModules->HasSelection()) {
        auto selection = registry.BtnsModules->Selected();
        if (auto* module = ModuleRegistry::GetInstance().Find(selection)) {
          renderDummy = false;
          ImGui::BeginChild(module->GetName().c_str(), ImVec2(area.x, area.y), true);
          module->Render();
          ImGui::EndChild();
        }
      }
    }
    if (renderDummy) {
      ImGui::BeginChild("ContentArea", ImVec2(area.x, area.y), true);
      ImGui::Text(Labels::BLANK_CONTENT_AREA);
      ImGui::EndChild();
    }
    ImGui::EndTable();
  }
}

void ContentPanel::ShowSidebarPanel(SideBarItem panel) {
  auto& registry = ModuleRegistry::GetInstance();

  bool canNavigate = false;
  switch (panel) {
  case SideBarItem::METADATA:
    canNavigate = true;
    break;
  case SideBarItem::API_TRACE:
    canNavigate = true;
    break;
  case SideBarItem::LOG:
    canNavigate = true;
    break;
  case SideBarItem::APP_LOG:
    canNavigate = true;
    break;
  case SideBarItem::LABEL_STREAM:
  case SideBarItem::LABEL_LOG:
  case SideBarItem::COUNT:
  default:
    break;
  };

  if (canNavigate && m_BtnsSideBar.IsEnabled(panel)) {
    if (registry.BtnsModules != nullptr) {
      registry.BtnsModules->DeselectEntry();
    }
    m_BtnsSideBar.SelectEntry(panel);
  }
}

void ContentPanel::StudioEventCallback(const Event& e) {
  auto& context = StudioContext::GetInstance();
  const StudioEvent& studioEvent = static_cast<const StudioEvent&>(e);
  if (studioEvent.EventType == StudioEvent::Type::LiveStreamModeChanged) {
    auto isStreamMode = !context.IsLiveMode();

    m_BtnsSideBar.SetVisible(SideBarItem::LABEL_STREAM, isStreamMode);
    m_BtnsSideBar.SetVisible(SideBarItem::METADATA, isStreamMode);
    m_BtnsSideBar.SetVisible(SideBarItem::API_TRACE, isStreamMode);
    m_BtnsSideBar.SetVisible(SideBarItem::LOG, isStreamMode);

    if (!isStreamMode) {
      if (m_BtnsSideBar.HasSelection()) {
        if (!m_BtnsSideBar.IsVisible(m_BtnsSideBar.Selected())) {
          m_BtnsSideBar.SelectEntry(SideBarItem::APP_LOG);
        }
      }
    }
  }
}

void ContentPanel::AppEventCallback(const Event& e) {
  auto& registry = ModuleRegistry::GetInstance();
  const AppEvent& appEvent = static_cast<const AppEvent&>(e);
  bool deselectModules = false;

  if (appEvent.EventType == AppEvent::Type::ShowGITSLog) {
    m_BtnsSideBar.SelectEntry(SideBarItem::LOG);
    deselectModules = true;
  } else if (appEvent.EventType == AppEvent::Type::ShowAppLog) {
    m_BtnsSideBar.SelectEntry(SideBarItem::APP_LOG);
    deselectModules = true;
  }

  if (deselectModules && registry.BtnsModules != nullptr) {
    registry.BtnsModules->DeselectEntry();
  }
}

void ContentPanel::ActionCallback(const Event& e) {
  const ActionEvent& actionEvent = static_cast<const ActionEvent&>(e);
  auto& context = StudioContext::GetInstance();
}

void ContentPanel::NavigateCallback(const Event& e) {
  const NavigateEvent& navigateEvent = static_cast<const NavigateEvent&>(e);
  if (navigateEvent.EventType == NavigateEvent::Type::NavigateToPanel) {
    ShowSidebarPanel(navigateEvent.Panel);
  }
}
} // namespace gits::gui
