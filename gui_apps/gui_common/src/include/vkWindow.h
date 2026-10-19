// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "vulkanLoader.h"
#include "imgui.h"

#ifdef _WIN32
#include "imgui_impl_win32.h"
#include "imgui_impl_vulkan.h"
#include <windows.h>
#include <shellapi.h>
#else
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include "log.h"
#include "baseConfig.h"
#include "windowEvents.h"

#ifdef _WIN32
// Forward declare Win32 ImGui handler and WndProc
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

namespace gits::gui {

static void GLFWErrorCallback(int error, const char* description);
static void CheckVKResult(VkResult err);
static bool IsExtensionAvailable(const std::vector<VkExtensionProperties>& properties,
                                 const char* extension);
class VKWindow {
public:
#ifdef _WIN32
  VKWindow(BaseConfig* config, WindowEvents* windowEvents, WORD appIcon, std::wstring windowTitle);
#else
  VKWindow(BaseConfig* config, WindowEvents* windowEvents, std::wstring windowTitle);
#endif
  ~VKWindow() = default;

  bool Setup();
  void CleanUp();

  void RestoreWindow();

  void NewFrame();
  void RenderFrame(ImDrawData* draw_data, ImVec4 clearColor);

  bool ProcessEvents();

  void OnWindowResized(int width, int height);
  void OnWindowMoved(int xpos, int ypos);
  void OnWindowFileDropped(const char* filePath);

private:
  bool SetupWindow();
  void CleanUpWindow();

  void SetupVulkan();
  void CleanUpVulkan();

  bool SetupVulkanWindow(int width, int height);
  void CleanUpVulkanWindow();

  void SetupImGUI();
  void CleanUpImGUI();

private:
  void FrameRender(ImDrawData* draw_data);
  void FramePresent();

#ifdef _WIN32
  static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
  LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

#ifdef _WIN32
  WORD m_APPIcon;
#endif
  std::wstring m_WindowTitle;
  BaseConfig* m_AppConfig = nullptr;
  WindowEvents* m_WindowEvents = nullptr;

  VkAllocationCallbacks* g_Allocator = nullptr;
  VkInstance g_Instance = VK_NULL_HANDLE;
  VkPhysicalDevice g_PhysicalDevice = VK_NULL_HANDLE;
  VkDevice g_Device = VK_NULL_HANDLE;
  uint32_t g_QueueFamily = (uint32_t)-1;
  VkQueue g_Queue = VK_NULL_HANDLE;
  VkDebugReportCallbackEXT g_DebugReport = VK_NULL_HANDLE;
  VkPipelineCache g_PipelineCache = VK_NULL_HANDLE;
  VkDescriptorPool g_DescriptorPool = VK_NULL_HANDLE;
  bool g_SwapChainRebuild = false;
  uint32_t g_MinImageCount = 2;
  ImGui_ImplVulkanH_Window g_MainWindowData;
#ifdef _WIN32
  HWND g_MainWindowHWnd = nullptr;
  WNDCLASSEXW g_WC;
#else
  GLFWwindow* g_GLFWWindow = nullptr;
#endif
};

} // namespace gits::gui
