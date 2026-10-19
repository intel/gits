// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "vkWindow.h"

#include <cstdlib>

namespace gits::gui {
void GLFWErrorCallback(int error, const char* description) {
  LOG_ERROR << "GLFW Error " << error << ": " << description;
}

void CheckVKResult(VkResult err) {
  if (err == VK_SUCCESS) {
    return;
  }
  LOG_ERROR << "[vulkan] Error: VkResult = " << err;
  if (err < 0) {
    abort();
  }
}

bool IsExtensionAvailable(const std::vector<VkExtensionProperties>& properties,
                          const char* extension) {
  for (const VkExtensionProperties& p : properties) {
    if (strcmp(p.extensionName, extension) == 0) {
      return true;
    }
  }
  return false;
}

#ifdef _WIN32
VKWindow::VKWindow(BaseConfig* config,
                   WindowEvents* windowEvents,
                   WORD appIcon,
                   std::wstring windowTitle)
    : m_APPIcon(appIcon),
      m_WindowTitle(std::move(windowTitle)),
      m_AppConfig(config),
      m_WindowEvents(windowEvents) {}
#else
VKWindow::VKWindow(BaseConfig* config, WindowEvents* windowEvents, std::wstring windowTitle)
    : m_WindowTitle(std::move(windowTitle)), m_AppConfig(config), m_WindowEvents(windowEvents) {}
#endif

#ifdef _WIN32
LRESULT CALLBACK VKWindow::StaticWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  VKWindow* pThis = nullptr;

  if (msg == WM_NCCREATE) {
    CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
    pThis = reinterpret_cast<VKWindow*>(pCreate->lpCreateParams);
    SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
  } else {
    pThis = reinterpret_cast<VKWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
  }

  if (pThis) {
    return pThis->WndProc(hWnd, msg, wParam, lParam);
  }

  return DefWindowProcW(hWnd, msg, wParam, lParam);
}

bool VKWindow::SetupWindow() {
  ImGui_ImplWin32_EnableDpiAwareness();

  static const wchar_t* CLASS_NAME = L"VKWindowClass";

  HMODULE moduleHandle = GetModuleHandle(nullptr);
  if (moduleHandle == nullptr) {
    LOG_ERROR << "Failed to get module handle for VKWindow. GetLastError=" << GetLastError();
    return false;
  }

  HICON hIcon = LoadIcon(moduleHandle, MAKEINTRESOURCE(m_APPIcon));
  if (hIcon == nullptr) {
    LOG_ERROR << "Failed to load VKWindow icon. Resource=" << m_APPIcon
              << ", GetLastError=" << GetLastError();
    return false;
  }

  g_WC = {sizeof(g_WC), CS_CLASSDC, &StaticWndProc, 0L,      0L,         moduleHandle,
          hIcon,        nullptr,    nullptr,        nullptr, CLASS_NAME, hIcon};

  const ATOM classAtom = RegisterClassExW(&g_WC);
  if (classAtom == 0) {
    const DWORD error = GetLastError();
    if (error != ERROR_CLASS_ALREADY_EXISTS) {
      LOG_ERROR << "RegisterClassExW FAILED. GetLastError=" << error;
      OutputDebugStringW(L"RegisterClassExW FAILED\n");
      return false;
    }
    LOG_VERBOSE << "VKWindow class already registered; continuing.";
  }

  g_MainWindowHWnd = CreateWindowW(
      g_WC.lpszClassName, m_WindowTitle.c_str(), WS_OVERLAPPEDWINDOW,
      static_cast<int>(m_AppConfig->WindowPos.x), static_cast<int>(m_AppConfig->WindowPos.y),
      static_cast<int>(m_AppConfig->WindowSize.x), static_cast<int>(m_AppConfig->WindowSize.y),
      nullptr, nullptr, g_WC.hInstance, this);

  SendMessage(g_MainWindowHWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
  SendMessage(g_MainWindowHWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
  DragAcceptFiles(g_MainWindowHWnd, TRUE);
  return true;
}

void VKWindow::CleanUpWindow() {
  DestroyWindow(g_MainWindowHWnd);
  UnregisterClassW(g_WC.lpszClassName, g_WC.hInstance);
}
#else
namespace {
bool EnvVarIsTrue(const char* name) {
  const char* value = std::getenv(name);
  if (value == nullptr) {
    return false;
  }
  const std::string stringValue(value);
  return stringValue == "1" || stringValue == "true" || stringValue == "TRUE" ||
         stringValue == "yes" || stringValue == "YES";
}

void window_size_callback(GLFWwindow* window, int width, int height) {
  auto* vkWindow = static_cast<VKWindow*>(glfwGetWindowUserPointer(window));
  if (vkWindow == nullptr) {
    return;
  }
  vkWindow->OnWindowResized(width, height);
}

void window_pos_callback(GLFWwindow* window, int xpos, int ypos) {
  auto* vkWindow = static_cast<VKWindow*>(glfwGetWindowUserPointer(window));
  if (vkWindow == nullptr) {
    return;
  }
  vkWindow->OnWindowMoved(xpos, ypos);
}

void window_drop_callback(GLFWwindow* window, int count, const char** paths) {
  if (count <= 0 || paths == nullptr || paths[0] == nullptr) {
    return;
  }
  auto* vkWindow = static_cast<VKWindow*>(glfwGetWindowUserPointer(window));
  if (vkWindow == nullptr) {
    return;
  }
  vkWindow->OnWindowFileDropped(paths[0]);
}
} // namespace

bool VKWindow::SetupWindow() {
  glfwSetErrorCallback(GLFWErrorCallback);

  const bool forceX11 = EnvVarIsTrue("GITS_FORCE_X11");
  if (forceX11) {
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
    LOG_INFO << "GLFW platform forced to X11 by GITS_FORCE_X11";
  } else {
    LOG_INFO << "GLFW platform auto-selection enabled";
  }

  glfwInitVulkanLoader(vkGetInstanceProcAddr);

  if (!glfwInit()) {
    LOG_ERROR << "Failed to initialize GLFW";
    return false;
  }
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  g_GLFWWindow = glfwCreateWindow(static_cast<int>(m_AppConfig->WindowSize.x),
                                  static_cast<int>(m_AppConfig->WindowSize.y),
                                  m_AppConfig->AppName.c_str(), nullptr, nullptr);
  if (g_GLFWWindow == nullptr) {
    LOG_ERROR << "Failed to create GLFW window";
    glfwTerminate();
    return false;
  }

  if (!glfwVulkanSupported()) {
    LOG_ERROR << "GLFW: Vulkan Not Supported";
    glfwDestroyWindow(g_GLFWWindow);
    g_GLFWWindow = nullptr;
    glfwTerminate();
    return false;
  }

  glfwSetWindowUserPointer(g_GLFWWindow, this);
  glfwSetWindowPosCallback(g_GLFWWindow, window_pos_callback);
  glfwSetWindowSizeCallback(g_GLFWWindow, window_size_callback);
  glfwSetDropCallback(g_GLFWWindow, window_drop_callback);

  RestoreWindow();
  return true;
}

void VKWindow::CleanUpWindow() {
  if (g_GLFWWindow != nullptr) {
    glfwDestroyWindow(g_GLFWWindow);
    g_GLFWWindow = nullptr;
  }
  glfwTerminate();
}
#endif

void VKWindow::SetupVulkan() {
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

  if (!LoadVulkanInstanceFunctions(g_Instance)) {
    LOG_ERROR << "Failed to load Vulkan Instance level functions";
    return;
  }

  ImGui_ImplVulkan_LoadFunctions(VK_VERSION_1_4, VulkanLoaderFunction, g_Instance);

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

    if (!LoadVulkanDeviceFunctions(g_Device)) {
      LOG_ERROR << "Failed to load Vulkan Device level functions";
      return;
    }

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

void VKWindow::CleanUpVulkan() {
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

bool VKWindow::SetupVulkanWindow(int width, int height) {
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

void VKWindow::CleanUpVulkanWindow() {
  ImGui_ImplVulkanH_DestroyWindow(g_Instance, g_Device, &g_MainWindowData, g_Allocator);
}

void VKWindow::SetupImGUI() {
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

void VKWindow::CleanUpImGUI() {
  ImGui_ImplVulkan_Shutdown();
#ifdef _WIN32
  ImGui_ImplWin32_Shutdown();
#else
  ImGui_ImplGlfw_Shutdown();
#endif
  ImGui::DestroyContext();
}

bool VKWindow::Setup() {
  if (!LoadVulkanLibrary()) {
    LOG_ERROR << "Failed to load Vulkan library";
    return false;
  }

  if (!SetupWindow()) {
    return false;
  }

  SetupVulkan();
  if (!SetupVulkanWindow(static_cast<int>(m_AppConfig->WindowSize.x),
                         static_cast<int>(m_AppConfig->WindowSize.y))) {
    LOG_ERROR << "Failed to setup Vulkan window.";
    return false;
  }
#ifdef _WIN32
  ShowWindow(g_MainWindowHWnd, SW_SHOWDEFAULT);
  UpdateWindow(g_MainWindowHWnd);
#endif
  SetupImGUI();

  return true;
}

void VKWindow::CleanUp() {
  CleanUpImGUI();
  CleanUpVulkanWindow();
  CleanUpVulkan();
  CleanUpWindow();
}
#pragma endregion

#pragma region main & render functions
void VKWindow::FrameRender(ImDrawData* draw_data) {
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

void VKWindow::FramePresent() {
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
LRESULT WINAPI VKWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
    return TRUE;
  }

  switch (msg) {
  case WM_SIZE:
    if (m_WindowEvents) {
      RECT rect;
      GetWindowRect(g_MainWindowHWnd, &rect);
      ImVec2 size = ImVec2(static_cast<float>(rect.right - rect.left),
                           static_cast<float>(rect.bottom - rect.top));

      m_WindowEvents->OnResize(size);
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
    if (m_WindowEvents) {
      RECT rect;
      GetWindowRect(g_MainWindowHWnd, &rect);
      ImVec2 position = ImVec2(static_cast<float>(rect.left), static_cast<float>(rect.top));
      m_WindowEvents->OnPosition(position);
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
  case WM_DROPFILES: {
    HDROP hDrop = reinterpret_cast<HDROP>(wParam);
    wchar_t filePath[MAX_PATH] = {};
    if (DragQueryFileW(hDrop, 0, filePath, MAX_PATH)) {
      if (m_WindowEvents) {
        m_WindowEvents->OnFileDrop(std::filesystem::path(filePath));
      }
    }
    DragFinish(hDrop);
    return 0;
  }
  default:
    break;
  }
  return DefWindowProcW(hWnd, msg, wParam, lParam);
}

bool VKWindow::ProcessEvents() {
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
bool VKWindow::ProcessEvents() {
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

void VKWindow::RestoreWindow() {
#ifdef _WIN32
  if (g_MainWindowHWnd == nullptr || m_AppConfig == nullptr) {
    return;
  }
  SetWindowPos(g_MainWindowHWnd, nullptr, static_cast<int>(m_AppConfig->WindowPos.x),
               static_cast<int>(m_AppConfig->WindowPos.y),
               static_cast<int>(m_AppConfig->WindowSize.x),
               static_cast<int>(m_AppConfig->WindowSize.y), SWP_NOZORDER | SWP_NOACTIVATE);
#else
  if (g_GLFWWindow == nullptr || m_AppConfig == nullptr) {
    return;
  }
  glfwSetWindowPos(g_GLFWWindow, static_cast<int>(m_AppConfig->WindowPos.x),
                   static_cast<int>(m_AppConfig->WindowPos.y));
  glfwSetWindowSize(g_GLFWWindow, static_cast<int>(m_AppConfig->WindowSize.x),
                    static_cast<int>(m_AppConfig->WindowSize.y));
#endif
}

void VKWindow::OnWindowResized(int width, int height) {
  if (m_WindowEvents == nullptr) {
    return;
  }
  m_WindowEvents->OnResize(ImVec2(static_cast<float>(width), static_cast<float>(height)));
}

void VKWindow::OnWindowMoved(int xpos, int ypos) {
  if (m_WindowEvents == nullptr) {
    return;
  }
  m_WindowEvents->OnPosition(ImVec2(static_cast<float>(xpos), static_cast<float>(ypos)));
}

void VKWindow::OnWindowFileDropped(const char* filePath) {
  if (m_WindowEvents == nullptr || filePath == nullptr || filePath[0] == '\0') {
    return;
  }
  m_WindowEvents->OnFileDrop(std::filesystem::path(filePath));
}

void VKWindow::NewFrame() {
  ImGui_ImplVulkan_NewFrame();
#ifdef _WIN32
  ImGui_ImplWin32_NewFrame();
#else
  ImGui_ImplGlfw_NewFrame();
#endif
  ImGui::NewFrame();
}

void VKWindow::RenderFrame(ImDrawData* draw_data, ImVec4 clearColor) {
  g_MainWindowData.ClearValue.color.float32[0] = clearColor.x;
  g_MainWindowData.ClearValue.color.float32[1] = clearColor.y;
  g_MainWindowData.ClearValue.color.float32[2] = clearColor.z;
  g_MainWindowData.ClearValue.color.float32[3] = clearColor.w;

  FrameRender(draw_data);
  FramePresent();
}

} // namespace gits::gui
