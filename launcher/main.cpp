// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "imgui.h"

#ifdef _WIN32
#include "imgui_impl_win32.h"
#define VK_USE_PLATFORM_WIN32_KHR
#include "imgui_impl_vulkan.h"
#include <windows.h>
#else
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "log.h"
#include "guiController.h"
#include "resource.h"
#include "context.h"

#ifdef _WIN32
// Forward declare Win32 ImGui handler and WndProc
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

#pragma region globals
// VK globals
static VkAllocationCallbacks* g_Allocator = nullptr;
static VkInstance g_Instance = VK_NULL_HANDLE;
static VkPhysicalDevice g_PhysicalDevice = VK_NULL_HANDLE;
static VkDevice g_Device = VK_NULL_HANDLE;
static uint32_t g_QueueFamily = (uint32_t)-1;
static VkQueue g_Queue = VK_NULL_HANDLE;
static VkDebugReportCallbackEXT g_DebugReport = VK_NULL_HANDLE;
static VkPipelineCache g_PipelineCache = VK_NULL_HANDLE;
static VkDescriptorPool g_DescriptorPool = VK_NULL_HANDLE;
static bool g_SwapChainRebuild = false;
static uint32_t g_MinImageCount = 2;
static ImGui_ImplVulkanH_Window g_MainWindowData;
#ifdef _WIN32
static HWND g_MainWindowHWnd = nullptr;
static WNDCLASSEXW g_WC;
#else
static GLFWwindow* g_GLFWWindow;
#endif

gits::gui::GUIController* g_GUI = nullptr;
gits::gui::LauncherConfig g_LauncherConfig = gits::gui::LauncherConfig();
#pragma endregion

#pragma region VK helper functions
static void GLFWErrorCallback(int error, const char* description) {
  LOG_ERROR << "GLFW Error " << error << ": " << description;
}

static void CheckVKResult(VkResult err) {
  if (err == VK_SUCCESS) {
    return;
  }
  LOG_ERROR << "[vulkan] Error: VkResult = " << err;
  if (err < 0) {
    abort();
  }
}

static bool IsExtensionAvailable(const std::vector<VkExtensionProperties>& properties,
                                 const char* extension) {
  for (const VkExtensionProperties& p : properties) {
    if (strcmp(p.extensionName, extension) == 0) {
      return true;
    }
  }
  return false;
}
#pragma endregion

#pragma region setup & cleanup functions
#ifdef _WIN32
static bool SetupWindow() {
  ImGui_ImplWin32_EnableDpiAwareness();

  HICON hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(APP_ICON));
  g_WC = {sizeof(g_WC),
          CS_CLASSDC,
          WndProc,
          0L,
          0L,
          GetModuleHandle(nullptr),
          hIcon,
          nullptr,
          nullptr,
          nullptr,
          gits::gui::Settings::WINDOW_TITLE_W,
          hIcon};
  RegisterClassExW(&g_WC);

  g_MainWindowHWnd = CreateWindowW(
      g_WC.lpszClassName, gits::gui::Settings::WINDOW_TITLE_W, WS_OVERLAPPEDWINDOW,
      static_cast<int>(g_LauncherConfig.WindowPos.x),
      static_cast<int>(g_LauncherConfig.WindowPos.y),
      static_cast<int>(g_LauncherConfig.WindowSize.x),
      static_cast<int>(g_LauncherConfig.WindowSize.y), nullptr, nullptr, g_WC.hInstance, nullptr);
  SendMessage(g_MainWindowHWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
  SendMessage(g_MainWindowHWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
  return true;
}

static void CleanUpWindow() {
  DestroyWindow(g_MainWindowHWnd);
  UnregisterClassW(g_WC.lpszClassName, g_WC.hInstance);
}
#else
static void window_size_callback(GLFWwindow* window, int width, int height) {
  if (g_GUI != nullptr) {
    g_GUI->Resized(width, height);
  }
}

static void window_pos_callback(GLFWwindow* window, int xpos, int ypos) {
  if (g_GUI != nullptr) {
    g_GUI->Positioned(xpos, ypos);
  }
}

static bool SetupWindow() {
  glfwSetErrorCallback(GLFWErrorCallback);

  // force X11 for now
  glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
  if (!glfwInit()) {
    return false;
  }
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  float main_scale = 1.0f;
  g_GLFWWindow = glfwCreateWindow((int)(g_LauncherConfig.WindowSize.x * main_scale),
                                  (int)(g_LauncherConfig.WindowSize.y * main_scale),
                                  gits::gui::Settings::WINDOW_TITLE, nullptr, nullptr);
  if (!glfwVulkanSupported()) {
    LOG_ERROR << "GLFW: Vulkan Not Supported";
    return false;
  }

  glfwSetWindowPosCallback(g_GLFWWindow, window_pos_callback);
  glfwSetWindowSizeCallback(g_GLFWWindow, window_size_callback);
  return true;
}

static void CleanUpWindow() {
  glfwDestroyWindow(g_GLFWWindow);
  glfwTerminate();
}

#endif
static void SetupVulkan() {
  std::vector<const char*> instance_extensions;
#ifdef _WIN32
  instance_extensions.push_back("VK_KHR_surface");
  instance_extensions.push_back("VK_KHR_win32_surface");
#else
  uint32_t extensions_count = 0;
  const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&extensions_count);
  for (uint32_t i = 0; i < extensions_count; i++) {
    instance_extensions.push_back(glfw_extensions[i]);
  }
#endif
  {

    VkResult err;
    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    uint32_t properties_count = 0;
    std::vector<VkExtensionProperties> properties;
    vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, nullptr);
    properties.resize(properties_count);
    err = vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, properties.data());
    CheckVKResult(err);

    if (IsExtensionAvailable(properties, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
      instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    }

    create_info.enabledExtensionCount = (uint32_t)instance_extensions.size();
    create_info.ppEnabledExtensionNames = instance_extensions.data();
    err = vkCreateInstance(&create_info, g_Allocator, &g_Instance);
    CheckVKResult(err);
  }
  g_PhysicalDevice = ImGui_ImplVulkanH_SelectPhysicalDevice(g_Instance);
  IM_ASSERT(g_PhysicalDevice != VK_NULL_HANDLE);
  g_QueueFamily = ImGui_ImplVulkanH_SelectQueueFamilyIndex(g_PhysicalDevice);
  IM_ASSERT(g_QueueFamily != (uint32_t)-1);

  {
    VkResult err;
    std::vector<const char*> device_extensions;
    device_extensions.push_back("VK_KHR_swapchain");

    uint32_t properties_count = 0;
    std::vector<VkExtensionProperties> properties;
    vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr, &properties_count, nullptr);
    properties.resize(properties_count);
    vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr, &properties_count,
                                         properties.data());

    const float queue_priority[] = {1.0f};
    VkDeviceQueueCreateInfo queue_info[1] = {};
    queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info[0].queueFamilyIndex = g_QueueFamily;
    queue_info[0].queueCount = 1;
    queue_info[0].pQueuePriorities = queue_priority;

    VkDeviceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = 1;
    create_info.pQueueCreateInfos = queue_info;
    create_info.enabledExtensionCount = (uint32_t)device_extensions.size();
    create_info.ppEnabledExtensionNames = device_extensions.data();

    err = vkCreateDevice(g_PhysicalDevice, &create_info, g_Allocator, &g_Device);
    CheckVKResult(err);
    vkGetDeviceQueue(g_Device, g_QueueFamily, 0, &g_Queue);
  }

  {
    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE},
    };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 0;
    for (VkDescriptorPoolSize& pool_size : pool_sizes) {
      pool_info.maxSets += pool_size.descriptorCount;
    }
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;
    VkResult res = vkCreateDescriptorPool(g_Device, &pool_info, g_Allocator, &g_DescriptorPool);
    CheckVKResult(res);
  }
}

static void CleanUpVulkan() {
  if (g_Device != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(g_Device, g_DescriptorPool, g_Allocator);
  }
  if (g_Device != VK_NULL_HANDLE) {
    vkDestroyDevice(g_Device, g_Allocator);
  }
  if (g_Instance != VK_NULL_HANDLE) {
    vkDestroyInstance(g_Instance, g_Allocator);
  }
}

static bool SetupVulkanWindow(int width, int height) {
  //ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  VkResult err = VK_SUCCESS;
#ifdef _WIN32
  VkWin32SurfaceCreateInfoKHR createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  createInfo.hwnd = g_MainWindowHWnd;
  createInfo.hinstance = ::GetModuleHandle(nullptr);
  err = vkCreateWin32SurfaceKHR(g_Instance, &createInfo, nullptr, &surface);
#else
  err = glfwCreateWindowSurface(g_Instance, g_GLFWWindow, g_Allocator, &surface);
  CheckVKResult(err);
  if (err == VK_SUCCESS) {
    int w, h;
    glfwGetFramebufferSize(g_GLFWWindow, &w, &h);
  }
#endif
  if (err != VK_SUCCESS) {
    LOG_ERROR << "Failed to create Vulkan surface.";
    return false;
  }

  g_MainWindowData.Surface = surface;

  VkBool32 supported = VK_FALSE;
  vkGetPhysicalDeviceSurfaceSupportKHR(g_PhysicalDevice, g_QueueFamily, g_MainWindowData.Surface,
                                       &supported);
  if (supported != VK_TRUE) {
    fprintf(stderr, "Error: no WSI support on physical device\n");
    return false;
  }

  const VkFormat requestSurfaceImageFormat[] = {VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM,
                                                VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM};
  const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
  g_MainWindowData.SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(
      g_PhysicalDevice, g_MainWindowData.Surface, requestSurfaceImageFormat,
      (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat), requestSurfaceColorSpace);
  VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_FIFO_KHR};
  g_MainWindowData.PresentMode = ImGui_ImplVulkanH_SelectPresentMode(
      g_PhysicalDevice, g_MainWindowData.Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));

  IM_ASSERT(g_MinImageCount >= 2);
  ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, &g_MainWindowData,
                                         g_QueueFamily, g_Allocator, width, height,
                                         g_MinImageCount);
  return true;
}

static void CleanUpVulkanWindow() {
  ImGui_ImplVulkanH_DestroyWindow(g_Instance, g_Device, &g_MainWindowData, g_Allocator);
}

static void SetupImGUI() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
  ImGui::StyleColorsDark();

#ifdef _WIN32
  ImGui_ImplWin32_Init(g_MainWindowHWnd);
#else
  ImGui_ImplGlfw_InitForVulkan(g_GLFWWindow, true);
#endif
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = g_Instance;
  init_info.PhysicalDevice = g_PhysicalDevice;
  init_info.Device = g_Device;
  init_info.QueueFamily = g_QueueFamily;
  init_info.Queue = g_Queue;
  init_info.PipelineCache = g_PipelineCache;
  init_info.DescriptorPool = g_DescriptorPool;
  init_info.RenderPass = g_MainWindowData.RenderPass;
  init_info.Subpass = 0;
  init_info.MinImageCount = g_MinImageCount;
  init_info.ImageCount = g_MainWindowData.ImageCount;
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  init_info.Allocator = g_Allocator;
  init_info.CheckVkResultFn = CheckVKResult;
  ImGui_ImplVulkan_Init(&init_info);
}

static void CleanUpImGUI() {
  ImGui_ImplVulkan_Shutdown();
#ifdef _WIN32
  ImGui_ImplWin32_Shutdown();
#else
  ImGui_ImplGlfw_Shutdown();
#endif
  ImGui::DestroyContext();
}

static void SetupGUI() {
#ifdef _WIN32
  g_GUI = new gits::gui::GUIController(g_MainWindowHWnd);
#else
  g_GUI = new gits::gui::GUIController(g_GLFWWindow);
#endif
  g_GUI->SetupGui();
}

static void CleanUpGUI() {
  if (g_GUI) {
    g_GUI->TeardownGui();
    delete g_GUI;
    g_GUI = nullptr;
  }
}

static bool Setup() {
  g_LauncherConfig = gits::gui::LauncherConfig::FromFile();

  if (!SetupWindow()) {
    return false;
  }

  SetupVulkan();
  if (!SetupVulkanWindow(static_cast<int>(g_LauncherConfig.WindowSize.x),
                         static_cast<int>(g_LauncherConfig.WindowSize.y))) {
    LOG_ERROR << "Failed to setup Vulkan window.";
    return false;
  }
#ifdef _WIN32
  ShowWindow(g_MainWindowHWnd, SW_SHOWDEFAULT);
  UpdateWindow(g_MainWindowHWnd);
#endif
  SetupImGUI();
  SetupGUI();
  g_GUI->RestoreWindow();

  return true;
}

static void CleanUp() {
  CleanUpGUI();
  CleanUpImGUI();
  CleanUpVulkanWindow();
  CleanUpVulkan();
  CleanUpWindow();
}
#pragma endregion

#pragma region main & render functions
static void FrameRender(ImDrawData* draw_data) {
  VkSemaphore image_acquired_semaphore =
      g_MainWindowData.FrameSemaphores[g_MainWindowData.SemaphoreIndex].ImageAcquiredSemaphore;
  VkSemaphore render_complete_semaphore =
      g_MainWindowData.FrameSemaphores[g_MainWindowData.SemaphoreIndex].RenderCompleteSemaphore;
  VkResult err =
      vkAcquireNextImageKHR(g_Device, g_MainWindowData.Swapchain, UINT64_MAX,
                            image_acquired_semaphore, VK_NULL_HANDLE, &g_MainWindowData.FrameIndex);
  if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
    g_SwapChainRebuild = true;
  }
  if (err == VK_ERROR_OUT_OF_DATE_KHR) {
    return;
  }
  if (err != VK_SUBOPTIMAL_KHR) {
    CheckVKResult(err);
  }

  ImGui_ImplVulkanH_Frame* fd = &g_MainWindowData.Frames[g_MainWindowData.FrameIndex];
  err = vkWaitForFences(g_Device, 1, &fd->Fence, VK_TRUE, UINT64_MAX);
  CheckVKResult(err);
  err = vkResetFences(g_Device, 1, &fd->Fence);
  CheckVKResult(err);

  err = vkResetCommandPool(g_Device, fd->CommandPool, 0);
  CheckVKResult(err);
  VkCommandBufferBeginInfo begin_info = {};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  err = vkBeginCommandBuffer(fd->CommandBuffer, &begin_info);
  CheckVKResult(err);

  VkRenderPassBeginInfo rp_info = {};
  rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  rp_info.renderPass = g_MainWindowData.RenderPass;
  rp_info.framebuffer = fd->Framebuffer;
  rp_info.renderArea.extent.width = g_MainWindowData.Width;
  rp_info.renderArea.extent.height = g_MainWindowData.Height;
  rp_info.clearValueCount = 1;
  rp_info.pClearValues = &g_MainWindowData.ClearValue;
  vkCmdBeginRenderPass(fd->CommandBuffer, &rp_info, VK_SUBPASS_CONTENTS_INLINE);

  ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

  vkCmdEndRenderPass(fd->CommandBuffer);

  VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkSubmitInfo submit_info = {};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = &image_acquired_semaphore;
  submit_info.pWaitDstStageMask = &wait_stage;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &fd->CommandBuffer;
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = &render_complete_semaphore;

  err = vkEndCommandBuffer(fd->CommandBuffer);
  CheckVKResult(err);
  err = vkQueueSubmit(g_Queue, 1, &submit_info, fd->Fence);
  CheckVKResult(err);
}

static void FramePresent() {
  if (g_SwapChainRebuild) {
    return;
  }
  VkSemaphore render_complete_semaphore =
      g_MainWindowData.FrameSemaphores[g_MainWindowData.SemaphoreIndex].RenderCompleteSemaphore;
  VkPresentInfoKHR info = {};
  info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  info.waitSemaphoreCount = 1;
  info.pWaitSemaphores = &render_complete_semaphore;
  info.swapchainCount = 1;
  info.pSwapchains = &g_MainWindowData.Swapchain;
  info.pImageIndices = &g_MainWindowData.FrameIndex;
  VkResult err = vkQueuePresentKHR(g_Queue, &info);
  if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
    g_SwapChainRebuild = true;
  }
  if (err == VK_ERROR_OUT_OF_DATE_KHR) {
    return;
  }
  if (err != VK_SUBOPTIMAL_KHR) {
    CheckVKResult(err);
  }
  g_MainWindowData.SemaphoreIndex =
      (g_MainWindowData.SemaphoreIndex + 1) % g_MainWindowData.SemaphoreCount;
}
#ifdef _WIN32
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
    return TRUE;
  }

  switch (msg) {
  case WM_SIZE:
    if (g_GUI) {
      g_GUI->Resized();
    }
    if (g_Device != VK_NULL_HANDLE && wParam != SIZE_MINIMIZED) {
      int fb_width = (UINT)LOWORD(lParam);
      int fb_height = (UINT)HIWORD(lParam);
      if (fb_width > 0 && fb_height > 0 &&
          (g_SwapChainRebuild || g_MainWindowData.Width != fb_width ||
           g_MainWindowData.Height != fb_height)) {
        ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
        ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device,
                                               &g_MainWindowData, g_QueueFamily, g_Allocator,
                                               fb_width, fb_height, g_MinImageCount);
        g_MainWindowData.FrameIndex = 0;
        g_SwapChainRebuild = false;
      }
    }
    return 0;
  case WM_MOVE:
    if (g_GUI) {
      g_GUI->Positioned();
    }
    break;
  case WM_SYSCOMMAND:
    if ((wParam & 0xfff0) == SC_KEYMENU) {
      return 0; // Disable ALT application menu
    }
    break;
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  default:
    break;
  }
  return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static bool ProcessEvents() {
  bool done = false;
  MSG msg;
  while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
    if (msg.message == WM_QUIT) {
      done = true;
    }
  }
  return done;
}
#else
static bool ProcessEvents() {
  if (glfwWindowShouldClose(g_GLFWWindow)) {
    return true;
  }
  glfwPollEvents();

  int fb_width, fb_height;
  glfwGetFramebufferSize(g_GLFWWindow, &fb_width, &fb_height);
  if (fb_width > 0 && fb_height > 0 &&
      (g_SwapChainRebuild || g_MainWindowData.Width != fb_width ||
       g_MainWindowData.Height != fb_height)) {
    ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
    ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device,
                                           &g_MainWindowData, g_QueueFamily, g_Allocator, fb_width,
                                           fb_height, g_MinImageCount);
    g_MainWindowData.FrameIndex = 0;
    g_SwapChainRebuild = false;
  }
  if (glfwGetWindowAttrib(g_GLFWWindow, GLFW_ICONIFIED) != 0) {
    ImGui_ImplGlfw_Sleep(10);
  }
  return false;
}
#endif

int main(int, char**) {
  gits::log::Initialize(gits::LogLevel::INFO);
  LOG_INFO << "Starting gitsLauncher GUI";

  if (!Setup()) {
    return 1;
  }

  bool done = false;
  while (!done) {
    g_GUI->UpdateUIScale();
    done = ProcessEvents();
    if (done || g_GUI->ShouldQuit) {
      break;
    }

    ImGui_ImplVulkan_NewFrame();
#ifdef _WIN32
    ImGui_ImplWin32_NewFrame();
#else
    ImGui_ImplGlfw_NewFrame();
#endif
    ImGui::NewFrame();

    g_GUI->DrawGui();

    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    const bool minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
    if (!minimized) {
      gits::gui::Settings::SetVKClearColor(&g_MainWindowData.ClearValue);
      FrameRender(draw_data);
      FramePresent();
    }
  }

  LOG_DEBUG << "Exiting application";

  VkResult res = vkDeviceWaitIdle(g_Device);
  CheckVKResult(res);

  CleanUp();
  return 0;
}
#pragma endregion
