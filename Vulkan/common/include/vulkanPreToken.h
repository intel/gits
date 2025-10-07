// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "vulkanArgumentsAuto.h"
#include "vulkanStateDynamic.h"
#include "vkWindowing.h"

namespace gits {
namespace Vulkan {
class CGitsVkCreateNativeWindow : public CFunction, gits::noncopyable {
  static const unsigned ARG_NUM = 7;
  Cint x, y, w, h;
  Cbool vis;
#if defined(GITS_PLATFORM_WINDOWS)
  CVkHWND hwnd;
  CVkHINSTANCE hinstance;
#endif
#if defined(GITS_PLATFORM_X11)
  CVkWindow hwnd;
  Cxcb_connection_t hinstance;
#endif

  virtual unsigned ArgumentCount() const {
    return ARG_NUM;
  }
  virtual CArgument& Argument(unsigned idx) {
    return get_cargument(__FUNCTION__, idx, x, y, w, h, vis, hwnd, hinstance);
  };
  static const std::array<ArgInfo, ARG_NUM> argumentInfos_;
  virtual ArgInfo ArgumentInfo(unsigned idx) const override;

public:
  CGitsVkCreateNativeWindow(){};
  CGitsVkCreateNativeWindow(HINSTANCE hinstance_in, HWND hwnd_in)
#if defined(GITS_PLATFORM_WINDOWS)
      : hwnd(hwnd_in), hinstance(hinstance_in) {
    window_handle win(hwnd_in);
    int xx, yy, ww, hh;
    win.get_dimensions(xx, yy, ww, hh);
    x.Value() = xx;
    y.Value() = yy;
    w.Value() = ww;
    h.Value() = hh;
    vis.Value() = win.is_visible();

    SD()._hwndstates.emplace(
        hwnd_in, std::make_shared<CHWNDState>(xx, yy, ww, hh, win.is_visible(), nullptr));
  }
#else
  {
  }
#endif
  CGitsVkCreateNativeWindow(HINSTANCE hinstance_in,
                            HWND hwnd_in,
                            int width,
                            int height,
                            int x_pos = 0,
                            int y_pos = 0,
                            bool visible = true)
#if defined(GITS_PLATFORM_WINDOWS)
      : hwnd(hwnd_in), hinstance(hinstance_in) {
    x.Value() = x_pos;
    y.Value() = y_pos;
    w.Value() = width;
    h.Value() = height;
    vis.Value() = visible;
    SD()._hwndstates.emplace(
        hwnd_in, std::make_shared<CHWNDState>(x_pos, y_pos, width, height, visible, nullptr));
  }
#else
  {
  }
#endif
  CGitsVkCreateNativeWindow(xcb_connection_t* connection, xcb_window_t window)
#if defined(GITS_PLATFORM_X11)
      : hwnd(window), hinstance(connection) {
    xcb_handle win(connection, window);
    int xx, yy, ww, hh;
    win.get_dimensions(xx, yy, ww, hh);
    x.Value() = xx;
    y.Value() = yy;
    w.Value() = ww;
    h.Value() = hh;
    vis.Value() = win.is_visible();

    SD()._hwndstates.emplace(
        window, std::make_shared<CHWNDState>(xx, yy, ww, hh, win.is_visible(), nullptr));
  }
#else
  {
  }
#endif

  const char* Name() const {
    return "CGitsVkCreateNativeWindow";
  }
  virtual void Write(CBinOStream& stream) const {
    x.Write(stream);
    y.Write(stream);
    w.Write(stream);
    h.Write(stream);
    vis.Write(stream);
    hwnd.Write(stream);
    hinstance.Write(stream);
  }
  virtual void Read(CBinIStream& stream) {
    x.Read(stream);
    y.Read(stream);
    w.Read(stream);
    h.Read(stream);
    vis.Read(stream);
    hwnd.Read(stream);
    hinstance.Read(stream);
  }

  virtual unsigned Id() const {
    return ID_GITS_VK_WINDOW_CREATOR;
  };

  virtual void Run() {
    bool visible = *vis;
    int width = *w;
    int height = *h;
    int xpos = *x;
    int ypos = *y;
    if (Configurator::Get().common.player.showWindowsWA) {
      visible = true;
    }
    if (Configurator::Get().common.player.forceWindowPos.enabled) {
      xpos = Configurator::Get().common.player.forceWindowPos.x;
      ypos = Configurator::Get().common.player.forceWindowPos.y;
    }
    if (Configurator::Get().common.player.forceWindowSize.enabled) {
      width = Configurator::Get().common.player.forceWindowSize.width;
      height = Configurator::Get().common.player.forceWindowSize.height;
    }
#if defined(GITS_PLATFORM_WINDOWS)
    Window_* win = new Window_(width, height, xpos, ypos, visible);
    SD()._hwndstates.emplace(win->handle(),
                             std::make_shared<CHWNDState>(xpos, ypos, width, height, visible, win));
    hwnd.AddMapping(win->handle());
    hinstance.AddMapping((HINSTANCE)GetModuleHandle(nullptr));
#elif defined(GITS_PLATFORM_X11)
    Window_* win = new Window_(width, height, xpos, ypos, visible, DisplayProtocol::XCB);
    SD()._hwndstates.emplace(win->handle(),
                             std::make_shared<CHWNDState>(xpos, ypos, width, height, visible, win));
    hwnd.AddMapping(win->handle());
    hinstance.AddMapping((xcb_connection_t*)win->native_display());
#else
    LOG_ERROR << "Vulkan window creation not implemented on this platform.";
    throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  }
  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> returnMap;
    for (auto obj : hwnd.GetMappedPointers()) {
      returnMap.insert((uint64_t)obj);
    }
    for (auto obj : hinstance.GetMappedPointers()) {
      returnMap.insert((uint64_t)obj);
    }
    return returnMap;
  }
};

class CGitsVkCreateXlibWindow : public CFunction, gits::noncopyable {
  static const unsigned ARG_NUM = 7;
  Cint x, y, w, h;
  Cbool vis;

  CVkWindow hwnd;
  CVkDisplay hinstance;

  virtual unsigned ArgumentCount() const {
    return ARG_NUM;
  }
  virtual CArgument& Argument(unsigned idx) {
    return get_cargument(__FUNCTION__, idx, x, y, w, h, vis, hwnd, hinstance);
  };
  static const std::array<ArgInfo, ARG_NUM> argumentInfos_;
  virtual ArgInfo ArgumentInfo(unsigned idx) const override;

public:
  CGitsVkCreateXlibWindow(){};
  CGitsVkCreateXlibWindow(Display* display, Window window)
#if defined(GITS_PLATFORM_X11)
      : hwnd(window), hinstance(display) {
    xlib_handle win(display, window);
    int xx, yy, ww, hh;
    win.get_dimensions(xx, yy, ww, hh);
    x.Value() = xx;
    y.Value() = yy;
    w.Value() = ww;
    h.Value() = hh;
    vis.Value() = win.is_visible();

    SD()._hwndstates.emplace(
        window, std::make_shared<CHWNDState>(xx, yy, ww, hh, win.is_visible(), nullptr));
  }
#else
  {
  }
#endif

  const char* Name() const {
    return "CGitsVkCreateXlibWindow";
  }
  virtual void Write(CBinOStream& stream) const {
    x.Write(stream);
    y.Write(stream);
    w.Write(stream);
    h.Write(stream);
    vis.Write(stream);
    hwnd.Write(stream);
    hinstance.Write(stream);
  }
  virtual void Read(CBinIStream& stream) {
    x.Read(stream);
    y.Read(stream);
    w.Read(stream);
    h.Read(stream);
    vis.Read(stream);
    hwnd.Read(stream);
    hinstance.Read(stream);
  }

  virtual unsigned Id() const {
    return ID_GITS_VK_XLIB_WINDOW_CREATOR;
  };

  virtual void Run() {
    bool visible = *vis;
    int width = *w;
    int height = *h;
    int xpos = *x;
    int ypos = *y;
    if (Configurator::Get().common.player.showWindowsWA) {
      visible = true;
    }
    if (Configurator::Get().common.player.forceWindowPos.enabled) {
      xpos = Configurator::Get().common.player.forceWindowPos.x;
      ypos = Configurator::Get().common.player.forceWindowPos.y;
    }
    if (Configurator::Get().common.player.forceWindowSize.enabled) {
      width = Configurator::Get().common.player.forceWindowSize.width;
      height = Configurator::Get().common.player.forceWindowSize.height;
    }

#if defined(GITS_PLATFORM_X11)
    Window_* win = new Window_(width, height, xpos, ypos, visible, DisplayProtocol::XLIB);
    SD()._hwndstates.emplace(win->handle(),
                             std::make_shared<CHWNDState>(xpos, ypos, width, height, visible, win));
    hwnd.AddMapping(win->handle());
    hinstance.AddMapping((Display*)win->native_display());
#else
    LOG_ERROR << "Vulkan XLIB window creation not implemented on this platform.";
    throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  }

  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> returnMap;
    for (auto obj : hwnd.GetMappedPointers()) {
      returnMap.insert((uint64_t)obj);
    }
    for (auto obj : hinstance.GetMappedPointers()) {
      returnMap.insert((uint64_t)obj);
    }
    return returnMap;
  }
};

class CGitsVkUpdateNativeWindow : public CFunction, gits::noncopyable {
  static const unsigned ARG_NUM = 6;
  Cint x, y, w, h;
  Cbool vis;
#if defined(GITS_PLATFORM_WINDOWS)
  CVkHWND hwnd;
#endif
#if defined(GITS_PLATFORM_X11)
  CVkWindow hwnd;
#endif

  virtual unsigned ArgumentCount() const {
    return ARG_NUM;
  }
  virtual CArgument& Argument(unsigned idx) {
    return get_cargument(__FUNCTION__, idx, x, y, w, h, vis, hwnd);
  };
  static const std::array<ArgInfo, ARG_NUM> argumentInfos_;
  virtual ArgInfo ArgumentInfo(unsigned idx) const override;
#if defined(GITS_PLATFORM_WINDOWS)
  void HelperSetValues(int xx, int yy, int ww, int hh, bool is_visible, HWND window) {
#elif defined(GITS_PLATFORM_X11)
  void HelperSetValues(int xx, int yy, int ww, int hh, bool is_visible, Window window) {
#endif
#if defined(GITS_PLATFORM_WINDOWS) || defined(GITS_PLATFORM_X11)
    x.Value() = xx;
    y.Value() = yy;
    w.Value() = ww;
    h.Value() = hh;
    vis.Value() = is_visible;

    auto& hwndState = SD()._hwndstates[window];

    hwndState->x = xx;
    hwndState->y = yy;
    hwndState->w = ww;
    hwndState->h = hh;
    hwndState->vis = is_visible;
  }
#endif

public:
  CGitsVkUpdateNativeWindow(){};
#if defined(GITS_PLATFORM_WINDOWS)
  CGitsVkUpdateNativeWindow(HWND hwnd_in) : hwnd(hwnd_in) {
    window_handle win(hwnd_in);
    int xx, yy, ww, hh;
    win.get_dimensions(xx, yy, ww, hh);
    HelperSetValues(xx, yy, ww, hh, win.is_visible(), hwnd_in);
  }
#endif

  CGitsVkUpdateNativeWindow(
      HWND hwnd_in, int width, int height, int x_pos = 0, int y_pos = 0, bool visible = true)
#if defined(GITS_PLATFORM_WINDOWS)
      : hwnd(hwnd_in) {
    HelperSetValues(x_pos, y_pos, width, height, visible, hwnd_in);
  }
#else
    {
    }
#endif
#if defined(GITS_PLATFORM_X11)
  CGitsVkUpdateNativeWindow(xcb_window_t window) : hwnd(window) {
    xcb_handle win((xcb_connection_t*)GetNativeDisplay(), window);
    int xx, yy, ww, hh;
    win.get_dimensions(xx, yy, ww, hh);
    HelperSetValues(xx, yy, ww, hh, win.is_visible(), window);
  }

  CGitsVkUpdateNativeWindow(Window window) : hwnd(window) {
    xlib_handle win((Display*)GetNativeDisplay(), window);
    int xx, yy, ww, hh;
    win.get_dimensions(xx, yy, ww, hh);
    HelperSetValues(xx, yy, ww, hh, win.is_visible(), window);
  }

#endif

  const char* Name() const {
    return "CGitsVkUpdateNativeWindow";
  }
  virtual void Write(CBinOStream& stream) const override {
    x.Write(stream);
    y.Write(stream);
    w.Write(stream);
    h.Write(stream);
    vis.Write(stream);
    hwnd.Write(stream);
  }
  virtual void Read(CBinIStream& stream) override {
    x.Read(stream);
    y.Read(stream);
    w.Read(stream);
    h.Read(stream);
    vis.Read(stream);
    hwnd.Read(stream);
  }

  virtual unsigned Id() const {
    return ID_GITS_VK_WINDOW_UPDATE;
  };

  virtual void Run() {
    bool visible = *vis;
    int width = *w;
    int height = *h;
    int xpos = *x;
    int ypos = *y;
    if (Configurator::Get().common.player.showWindowsWA) {
      visible = true;
    }
    if (Configurator::Get().common.player.forceWindowPos.enabled) {
      xpos = Configurator::Get().common.player.forceWindowPos.x;
      ypos = Configurator::Get().common.player.forceWindowPos.y;
    }
    if (Configurator::Get().common.player.forceWindowSize.enabled) {
      width = Configurator::Get().common.player.forceWindowSize.width;
      height = Configurator::Get().common.player.forceWindowSize.height;
    }
#if defined(GITS_PLATFORM_WINDOWS) || defined(GITS_PLATFORM_X11)
    Window_* win = SD()._hwndstates[*hwnd]->window.get();
    win->set_size(width, height);
    win->set_position(xpos, ypos);
    win->set_visibility(visible);
#else
      LOG_ERROR << "Vulkan window updates not implemented on this platform.";
      throw ENotImplemented(EXCEPTION_MESSAGE);
#endif
  }
  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> returnMap;
    for (auto obj : hwnd.GetMappedPointers()) {
      returnMap.insert((uint64_t)obj);
    }
    return returnMap;
  }
};

class CGitsVkMemoryUpdate : public CFunction, gits::noncopyable {
  static const unsigned ARG_NUM = 5;
  std::unique_ptr<CVkDevice> _device;
  std::unique_ptr<CVkDeviceMemory> _mem;
  std::unique_ptr<Cuint64_t> _offset;
  std::unique_ptr<Cuint64_t> _length;
  std::unique_ptr<CBinaryResource> _resource;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return ARG_NUM;
  }
  static const std::array<ArgInfo, ARG_NUM> argumentInfos_;
  virtual ArgInfo ArgumentInfo(unsigned idx) const override;

public:
  CGitsVkMemoryUpdate();
  CGitsVkMemoryUpdate(VkDevice device, VkDeviceMemory mem, bool unmap = false);
  virtual void GetDiffSubRange(const std::vector<char>& oldData,
                               const std::vector<char>& newRangeData,
                               std::uint64_t& length,
                               std::uint64_t& offset);

  virtual unsigned Id() const {
    return ID_GITS_VK_MEMORY_UPDATE;
  }
  virtual const char* Name() const {
    return "CGitsVkMemoryUpdate";
  }
  virtual void Write(CBinOStream& stream) const override;
  virtual void Read(CBinIStream& stream) override;
  virtual void Run();
  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> returnMap;
    for (auto obj : _device->GetMappedPointers()) {
      returnMap.insert((uint64_t)obj);
    }
    for (auto obj : _mem->GetMappedPointers()) {
      returnMap.insert((uint64_t)obj);
    }
    return returnMap;
  }
};

template <class T_WRAP>
class CVectorPrintHelper {
  const std::vector<std::shared_ptr<T_WRAP>>& vec_;

public:
  CVectorPrintHelper(const std::vector<std::shared_ptr<T_WRAP>>& vec) : vec_(vec) {}
  intptr_t ScopeKey() const {
    return reinterpret_cast<intptr_t>(this);
  }
};

class CGitsVkMemoryUpdate2 : public CFunction, gits::noncopyable {
  static const unsigned ARG_NUM = 5;
  std::unique_ptr<CVkDeviceMemory> _mem;
  std::unique_ptr<Cuint64_t> _size;
  std::vector<std::shared_ptr<Cuint64_t>> _offset;
  std::vector<std::shared_ptr<Cuint64_t>> _length;
  std::vector<std::shared_ptr<CBinaryResource>> _resource;

  virtual CArgument& Argument(unsigned idx) override;
  virtual unsigned ArgumentCount() const override {
    return ARG_NUM;
  }
  static const std::array<ArgInfo, ARG_NUM> argumentInfos_;
  virtual ArgInfo ArgumentInfo(unsigned idx) const override;

public:
  CGitsVkMemoryUpdate2();
  CGitsVkMemoryUpdate2(VkDeviceMemory memory, uint32_t regionCount, const VkBufferCopy* pRegions);

  virtual unsigned Id() const override {
    return ID_GITS_VK_MEMORY_UPDATE2;
  }
  virtual const char* Name() const override {
    return "CGitsVkMemoryUpdate2";
  }
  virtual void Write(CBinOStream& stream) const override;
  virtual void Read(CBinIStream& stream) override;
  virtual void Run() override;
  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> returnMap;
    for (auto obj : _mem->GetMappedPointers()) {
      returnMap.insert((uint64_t)obj);
    }
    return returnMap;
  }
};

class CGitsVkMemoryRestore : public CFunction, gits::noncopyable {
  static const unsigned ARG_NUM = 5;
  std::unique_ptr<CVkDevice> _device;
  std::unique_ptr<CVkDeviceMemory> _mem;
  std::unique_ptr<Cuint64_t> _length;
  std::unique_ptr<Cuint64_t> _offset;
  std::unique_ptr<CBinaryResource> _resource;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return ARG_NUM;
  }
  static const std::array<ArgInfo, ARG_NUM> argumentInfos_;
  virtual ArgInfo ArgumentInfo(unsigned idx) const override;

public:
  CGitsVkMemoryRestore();
  CGitsVkMemoryRestore(VkDevice device, VkDeviceMemory mem, std::uint64_t size);
  CGitsVkMemoryRestore(VkDevice device,
                       VkDeviceMemory mem,
                       std::uint64_t size,
                       const void* mappedPtr);
  virtual void GetDiffFromZero(const std::vector<char>& oldData,
                               std::uint64_t& length,
                               std::uint64_t& offset);

  virtual unsigned Id() const {
    return ID_GITS_VK_MEMORY_RESTORE;
  }
  virtual const char* Name() const {
    return "CGitsVkMemoryRestore";
  }
  virtual void Write(CBinOStream& stream) const override;
  virtual void Read(CBinIStream& stream) override;
  virtual void Run();
  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> returnMap;
    for (auto obj : _device->GetMappedPointers()) {
      returnMap.insert((uint64_t)obj);
    }
    for (auto obj : _mem->GetMappedPointers()) {
      returnMap.insert((uint64_t)obj);
    }
    return returnMap;
  }
};

class CGitsVkMemoryReset : public CFunction, gits::noncopyable {
  static const unsigned ARG_NUM = 3;
  std::unique_ptr<CVkDevice> _device;
  std::unique_ptr<CVkDeviceMemory> _mem;
  std::unique_ptr<Cuint64_t> _length;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return ARG_NUM;
  }
  static const std::array<ArgInfo, ARG_NUM> argumentInfos_;
  virtual ArgInfo ArgumentInfo(unsigned idx) const override;

public:
  CGitsVkMemoryReset();
  CGitsVkMemoryReset(VkDevice device, VkDeviceMemory mem, std::uint64_t size, void* mappedPtr);

  virtual unsigned Id() const {
    return ID_GITS_VK_MEMORY_RESET;
  }
  virtual const char* Name() const {
    return "CGitsVkMemoryReset";
  }
  virtual void Write(CBinOStream& stream) const override;
  virtual void Read(CBinIStream& stream) override;
  virtual void Run();
  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> returnMap;
    for (auto obj : _device->GetMappedPointers()) {
      returnMap.insert((uint64_t)obj);
    }
    for (auto obj : _mem->GetMappedPointers()) {
      returnMap.insert((uint64_t)obj);
    }
    return returnMap;
  }
};

class CGitsVkCmdPatchDeviceAddresses : public CFunction {
  static const unsigned ARG_NUM = 3;
  std::unique_ptr<Cuint32_t> _count;
  std::unique_ptr<CVkCommandBuffer> _commandBuffer;
  std::unique_ptr<CBinaryResource>
      _resource; // An array of VkBufferDeviceAddressPatchGITS structures

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return ARG_NUM;
  }
  static const std::array<ArgInfo, ARG_NUM> argumentInfos_;
  virtual ArgInfo ArgumentInfo(unsigned idx) const override;

public:
  CGitsVkCmdPatchDeviceAddresses();
  CGitsVkCmdPatchDeviceAddresses(VkCommandBuffer commandBuffer,
                                 CDeviceAddressPatcher& patcher,
                                 uint64_t commandId);

  virtual unsigned Id() const {
    return ID_GITS_VK_CMD_PATCH_DEVICE_ADDRESSES;
  }
  virtual const char* Name() const {
    return "CGitsVkCmdPatchDeviceAddresses";
  }
  virtual void Write(CBinOStream& stream) const override;
  virtual void Read(CBinIStream& stream) override;
  virtual void Run();
  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> returnMap;
    for (auto obj : _commandBuffer->GetMappedPointers()) {
      returnMap.insert((uint64_t)obj);
    }
    return returnMap;
  }
};

class CDestroyVulkanDescriptorSets : public CFunction, gits::noncopyable {
  CVkDescriptorSet::CSArray _descSetsArray;

  virtual CArgument& Argument(unsigned idx) override;
  virtual unsigned ArgumentCount() const override {
    return 1;
  }
  static const std::array<ArgInfo, 1> argumentInfos_;
  virtual ArgInfo ArgumentInfo(unsigned idx) const override;

public:
  CDestroyVulkanDescriptorSets();
  CDestroyVulkanDescriptorSets(size_t count, VkDescriptorSet* descSetsArray);
  virtual unsigned Id() const override {
    return ID_VK_DESTROY_VULKAN_DESCRIPTOR_SETS;
  }
  virtual const char* Name() const override {
    return "CGitsDestroyVulkanDescriptorSets";
  }
  virtual void Run() override;
  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> returnMap;
    for (auto obj : _descSetsArray.GetMappedPointers()) {
      returnMap.insert((uint64_t)obj);
    }
    return returnMap;
  }
};

class CDestroyVulkanCommandBuffers : public CFunction, gits::noncopyable {
  CVkCommandBuffer::CSArray _cmdBufsArray;

  virtual CArgument& Argument(unsigned idx) override;
  virtual unsigned ArgumentCount() const override {
    return 1;
  }
  static const std::array<ArgInfo, 1> argumentInfos_;
  virtual ArgInfo ArgumentInfo(unsigned idx) const override;

public:
  CDestroyVulkanCommandBuffers();
  CDestroyVulkanCommandBuffers(size_t count, VkCommandBuffer* cmdBufsArray);
  virtual unsigned Id() const override {
    return ID_VK_DESTROY_VULKAN_COMMAND_BUFFERS;
  }
  virtual const char* Name() const override {
    return "CGitsDestroyVulkanCommandBuffers";
  }
  virtual void Run() override;
  std::set<uint64_t> GetMappedPointers() override {
    std::set<uint64_t> returnMap;
    for (auto obj : _cmdBufsArray.GetMappedPointers()) {
      returnMap.insert((uint64_t)obj);
    }
    return returnMap;
  }
};

class CGitsVkEnumerateDisplayMonitors : public CFunction, gits::noncopyable {
  static const unsigned ARG_NUM = 1;
  CVkHMONITOR::CSMapArray _monitors;

  virtual CArgument& Argument(unsigned idx) override;
  virtual unsigned ArgumentCount() const override {
    return ARG_NUM;
  }
  static const std::array<ArgInfo, ARG_NUM> argumentInfos_;
  virtual ArgInfo ArgumentInfo(unsigned idx) const override;

public:
  CGitsVkEnumerateDisplayMonitors();
  CGitsVkEnumerateDisplayMonitors(bool);

  virtual unsigned Id() const override {
    return ID_GITS_VK_ENUMERATE_DISPLAY_MONITORS;
  }
  virtual const char* Name() const override {
    return "CGitsVkEnumerateDisplayMonitors";
  }
  virtual void Write(CBinOStream& stream) const override;
  virtual void Read(CBinIStream& stream) override;
  virtual void Run() override;
  std::set<uint64_t> GetMappedPointers() override {
    std::set<uint64_t> returnMap;
    for (auto obj : _monitors.GetMappedPointers()) {
      returnMap.insert((uint64_t)obj);
    }
    return returnMap;
  }
};

class CGitsInitializeImage : public CFunction, gits::noncopyable {
  static const unsigned ARG_NUM = 26;
  CVkCommandBuffer _commandBuffer;

  Cuint32_t _preSrcStageMask;
  Cuint32_t _preDstStageMask;
  Cuint32_t _preDependencyFlags;
  Cuint32_t _preMemoryBarrierCount;
  CVkMemoryBarrierArray _prePMemoryBarriers;
  Cuint32_t _preBufferMemoryBarrierCount;
  CVkBufferMemoryBarrierArray _prePBufferMemoryBarriers;
  Cuint32_t _preImageMemoryBarrierCount;
  CVkImageMemoryBarrierArray _prePImageMemoryBarriers;

  CVkBuffer _copySrcBuffer;
  CVkImage _copyDstImage;
  CVkImageLayout _copyDstImageLayout;
  Cuint32_t _copyRegionCount;
  CVkBufferImageCopyArray _copyPRegions;
  Cuint32_t _initializeRegionCount;
  CVkInitializeImageGITSArray _initializePRegions;

  Cuint32_t _postSrcStageMask;
  Cuint32_t _postDstStageMask;
  Cuint32_t _postDependencyFlags;
  Cuint32_t _postMemoryBarrierCount;
  CVkMemoryBarrierArray _postPMemoryBarriers;
  Cuint32_t _postBufferMemoryBarrierCount;
  CVkBufferMemoryBarrierArray _postPBufferMemoryBarriers;
  Cuint32_t _postImageMemoryBarrierCount;
  CVkImageMemoryBarrierArray _postPImageMemoryBarriers;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return ARG_NUM;
  }
  static const std::array<ArgInfo, ARG_NUM> argumentInfos_;
  virtual ArgInfo ArgumentInfo(unsigned idx) const override;

public:
  CGitsInitializeImage();

  virtual unsigned Id() const {
    return ID_GITS_VK_CMD_INITIALIZE_IMAGE_INTEL;
  }
  virtual const char* Name() const {
    return "CGitsInitializeImage";
  }
  virtual void Run();
  virtual void Exec();
  virtual void StateTrack();
  virtual void TokenBuffersUpdate();
  std::set<uint64_t> GetMappedPointers();
};

class CGitsVkCmdInsertMemoryBarriers : public CFunction, gits::noncopyable {
  static const unsigned ARG_NUM = 10;
  CVkCommandBuffer _commandBuffer;

  Cuint32_t _SrcStageMask;
  Cuint32_t _DstStageMask;
  Cuint32_t _DependencyFlags;
  Cuint32_t _MemoryBarrierCount;
  CVkMemoryBarrierArray _PMemoryBarriers;
  Cuint32_t _BufferMemoryBarrierCount;
  CVkBufferMemoryBarrierArray _PBufferMemoryBarriers;
  Cuint32_t _ImageMemoryBarrierCount;
  CVkImageMemoryBarrierArray _PImageMemoryBarriers;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return ARG_NUM;
  }
  static const std::array<ArgInfo, ARG_NUM> argumentInfos_;
  virtual ArgInfo ArgumentInfo(unsigned idx) const override;

public:
  CGitsVkCmdInsertMemoryBarriers();
  CGitsVkCmdInsertMemoryBarriers(VkCommandBuffer commandBuffer,
                                 VkPipelineStageFlags SrcStageMask,
                                 VkPipelineStageFlags DstStageMask,
                                 VkDependencyFlags DependencyFlags,
                                 uint32_t MemoryBarrierCount,
                                 const VkMemoryBarrier* PMemoryBarriers,
                                 uint32_t BufferMemoryBarrierCount,
                                 const VkBufferMemoryBarrier* PBufferMemoryBarriers,
                                 uint32_t ImageMemoryBarrierCount,
                                 const VkImageMemoryBarrier* PImageMemoryBarriers);

  virtual unsigned Id() const {
    return ID_GITS_VK_CMD_INSERT_MEMORY_BARRIERS;
  }
  virtual const char* Name() const {
    return "CGitsVkCmdInsertMemoryBarriers";
  }
  virtual void Run();
  virtual void Exec();
  virtual void StateTrack();
  virtual void TokenBuffersUpdate();
  std::set<uint64_t> GetMappedPointers();
};

class CGitsVkCmdInsertMemoryBarriers2 : public CFunction, gits::noncopyable {
  static const unsigned ARG_NUM = 2;
  CVkCommandBuffer _commandBuffer;
  CVkDependencyInfo _dependencyInfo;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return ARG_NUM;
  }
  static const std::array<ArgInfo, ARG_NUM> argumentInfos_;
  virtual ArgInfo ArgumentInfo(unsigned idx) const override;

public:
  CGitsVkCmdInsertMemoryBarriers2();
  CGitsVkCmdInsertMemoryBarriers2(VkCommandBuffer commandBuffer,
                                  const VkDependencyInfo* pDependencyInfo);

  virtual unsigned Id() const {
    return ID_GITS_VK_CMD_INSERT_MEMORY_BARRIERS_2;
  }
  virtual const char* Name() const {
    return "CGitsVkCmdInsertMemoryBarriers2";
  }
  virtual void Run();
  virtual void Exec();
  virtual void StateTrack();
  virtual void TokenBuffersUpdate();
  std::set<uint64_t> GetMappedPointers();
};

class CGitsInitializeMultipleImages : public CFunction, gits::noncopyable {
  static const unsigned ARG_NUM = 4;

  CVkCommandBuffer _commandBuffer;
  CVkBuffer _copySrcBuffer;
  Cuint32_t _imagesCount;
  CVkInitializeImageDataGITSArray _pInitializeImages;
  std::unique_ptr<CVkBufferMemoryBarrierData> _copyFromBufferMemoryBarrierPre;
  std::unique_ptr<CVkBufferMemoryBarrierData> _copyFromBufferMemoryBarrierPost;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return ARG_NUM;
  }
  static const std::array<ArgInfo, ARG_NUM> argumentInfos_;
  virtual ArgInfo ArgumentInfo(unsigned idx) const override;

public:
  CGitsInitializeMultipleImages();
  CGitsInitializeMultipleImages(VkCommandBuffer commandBuffer,
                                VkBuffer copySrcBuffer,
                                std::vector<VkInitializeImageDataGITS> const& initializeImages);

  virtual unsigned Id() const {
    return ID_GITS_VK_CMD_INITIALIZE_MULTIPLE_IMAGES_INTEL;
  }
  virtual const char* Name() const {
    return "CGitsInitializeMultipleImages";
  }
  virtual void Run();
  virtual void Exec();
  virtual void StateTrack();
  virtual void TokenBuffersUpdate();
  std::set<uint64_t> GetMappedPointers();
};

class CGitsInitializeBuffer : public CFunction, gits::noncopyable {
  static const unsigned ARG_NUM = 23;
  CVkCommandBuffer _commandBuffer;

  Cuint32_t _preSrcStageMask;
  Cuint32_t _preDstStageMask;
  Cuint32_t _preDependencyFlags;
  Cuint32_t _preMemoryBarrierCount;
  CVkMemoryBarrierArray _prePMemoryBarriers;
  Cuint32_t _preBufferMemoryBarrierCount;
  CVkBufferMemoryBarrierArray _prePBufferMemoryBarriers;
  Cuint32_t _preImageMemoryBarrierCount;
  CVkImageMemoryBarrierArray _prePImageMemoryBarriers;

  CVkBuffer _dataSrcBuffer;
  CVkBuffer _dataDstBuffer;
  Cuint32_t _dataRegionCount;
  CVkBufferCopyArray _dataPRegions;

  Cuint32_t _postSrcStageMask;
  Cuint32_t _postDstStageMask;
  Cuint32_t _postDependencyFlags;
  Cuint32_t _postMemoryBarrierCount;
  CVkMemoryBarrierArray _postPMemoryBarriers;
  Cuint32_t _postBufferMemoryBarrierCount;
  CVkBufferMemoryBarrierArray _postPBufferMemoryBarriers;
  Cuint32_t _postImageMemoryBarrierCount;
  CVkImageMemoryBarrierArray _postPImageMemoryBarriers;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return ARG_NUM;
  }
  static const std::array<ArgInfo, ARG_NUM> argumentInfos_;
  virtual ArgInfo ArgumentInfo(unsigned idx) const override;

public:
  CGitsInitializeBuffer();

  virtual unsigned Id() const {
    return ID_GITS_VK_CMD_INITIALIZE_BUFFER_INTEL;
  }
  virtual const char* Name() const {
    return "CGitsInitializeBuffer";
  }
  virtual void Run();
  virtual void Exec();
  virtual void StateTrack();
  virtual void TokenBuffersUpdate();
  std::set<uint64_t> GetMappedPointers();
};

class CGitsInitializeMultipleBuffers : public CFunction, gits::noncopyable {
  static const unsigned ARG_NUM = 4;

  CVkCommandBuffer _commandBuffer;
  CVkBuffer _copySrcBuffer;
  Cuint32_t _buffersCount;
  CVkInitializeBufferDataGITSArray _pInitializeBuffers;
  std::unique_ptr<CVkBufferMemoryBarrierData> _copyFromBufferMemoryBarrierPre;
  std::unique_ptr<CVkBufferMemoryBarrierData> _copyFromBufferMemoryBarrierPost;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return ARG_NUM;
  }
  static const std::array<ArgInfo, ARG_NUM> argumentInfos_;
  virtual ArgInfo ArgumentInfo(unsigned idx) const override;

public:
  CGitsInitializeMultipleBuffers();
  CGitsInitializeMultipleBuffers(VkCommandBuffer commandBuffer,
                                 VkBuffer copySrcBuffer,
                                 std::vector<VkInitializeBufferDataGITS> const& initializeBuffers);

  virtual unsigned Id() const {
    return ID_GITS_VK_CMD_INITIALIZE_MULTIPLE_BUFFERS_INTEL;
  }
  virtual const char* Name() const {
    return "CGitsInitializeMultipleBuffers";
  }
  virtual void Run();
  virtual void Exec();
  virtual void StateTrack();
  virtual void TokenBuffersUpdate();
  std::set<uint64_t> GetMappedPointers();
};

class CGitsVkStateRestoreInfo : public CFunction, gits::noncopyable {
  static const unsigned ARG_NUM = 3;
  std::unique_ptr<Cchar::CSArray> _phaseInfo;
  Cint _timerIndex;
  Cbool _timerOn;

  virtual CArgument& Argument(unsigned idx);
  virtual unsigned ArgumentCount() const {
    return ARG_NUM;
  }
  static const std::array<ArgInfo, ARG_NUM> argumentInfos_;
  virtual ArgInfo ArgumentInfo(unsigned idx) const override;

public:
  CGitsVkStateRestoreInfo();
  CGitsVkStateRestoreInfo(const char* phaseInfo);
  CGitsVkStateRestoreInfo(const char* phaseInfo, int index);

  virtual unsigned Id() const {
    return ID_GITS_VK_STATE_RESTORE_INFO;
  }
  virtual const char* Name() const {
    return "CGitsVkStateRestoreInfo";
  }
  virtual void Write(CBinOStream& stream) const override;
  virtual void Read(CBinIStream& stream) override;
  virtual void Run() override;
  std::set<uint64_t> GetMappedPointers() {
    std::set<uint64_t> returnMap;
    return returnMap;
  }
};
} // namespace Vulkan
} // namespace gits
