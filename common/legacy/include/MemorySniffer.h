// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "pragmas.h"

#include <iostream>
#include <map>
#include <set>
#include <exception>
#include <stdint.h>
#include <mutex>
#include <vector>
#ifndef _DEBUG
#ifndef NDEBUG
#define NDEBUG 1
#endif
#endif
#include <cassert>
#ifdef GITS_PLATFORM_WINDOWS
typedef void* PVOID;
#endif

// ******************************************************************************************************************
//
// Tools
//
// ******************************************************************************************************************

unsigned GetVirtualMemoryPageSize();

void* GetPage(void* addr);
const void* GetPage(const void* addr);

enum class PageMemoryProtection {
  READ_ONLY,
  READ_WRITE,
  WRITE_ONLY,
  NONE
};
bool SetPagesProtection(PageMemoryProtection access, void* ptr, size_t size = 1);

class MemorySniffer;
// ******************************************************************************************************************
//
// PagedMemoryRegion - this class represents a continuous Region of virtual memory
// and, if protected, contains a list of it's recently accessed memory pages. This class may be constructed, copied and
// assigned only through MemorySniffer who is a friend of this class.
//
// ******************************************************************************************************************
class PagedMemoryRegion {
public:
  typedef std::set<const void*> TouchedPages;
  friend class MemorySniffer;

private:
  const void* _ptr;
  size_t _size;
  bool _protected;
  TouchedPages _touchedPages;

  PagedMemoryRegion(const void* ptr, size_t size) : _ptr(ptr), _size(size), _protected(false) {}
  // PagedMemoryRegion(PagedMemoryRegion const&) = default;
  // PagedMemoryRegion& operator = (PagedMemoryRegion const&) = default;
  void TouchPageInternal(const void* ptr);

public:
  const void* BeginAddress() const {
    return _ptr;
  }
  const void* EndAddress() const {
    return (const char*)_ptr + _size;
  }
  size_t Size() const {
    return (size_t)_size;
  }
  const void* BeginPage() const {
    return GetPage(BeginAddress());
  }
  const void* EndPage() const {
    if (((uint64_t)EndAddress() % GetVirtualMemoryPageSize()) > 0) {
      return GetPage(EndAddress());
    } else {
      return (const void*)((const char*)EndAddress() - GetVirtualMemoryPageSize());
    }
  }
  size_t SizeOfPages() const {
    return (size_t)EndPage() - (size_t)BeginPage() + GetVirtualMemoryPageSize();
  }
  bool Protected() const {
    return _protected;
  }
  const TouchedPages GetTouchedPages() const;
  const TouchedPages GetTouchedPagesAndReset();
  void Reset();

  bool operator<(const PagedMemoryRegion& cmp) const {
    return _ptr < cmp._ptr;
  }
};

typedef PagedMemoryRegion* PagedMemoryRegionPtr;
//Handle is a unique pointer that allow class user to access PagedMemoryRange objects.
//If handle points to the null PagedMemoryRegionPtr it means underlaying region has been removed.
typedef PagedMemoryRegionPtr* PagedMemoryRegionHandle;

// ******************************************************************************************************************
//
// WriteWatchSniffer - this class is used to get touched/modified regions of memory allocated using WriteWatch
//
// ******************************************************************************************************************
class WriteWatchSniffer {
public:
  static std::vector<std::pair<char*, size_t>> GetTouchedPagesAndReset(char* ptr, size_t size);
  static void ResetTouchedPages(void* ptr, size_t size);
};

// ******************************************************************************************************************
//
// MemorySniffer - Creates and tracks PagedMemoryRegion-s.
// Allows users to operate on regions using handles interface.
// Allows to protect tracked regions from write operations.
// !! Passed regions shouldn't overlap pages that may contain unknown data because it may cause an undefined behavior. !!
// !! Unknown data may be for example a part of executed application heap (stl containers etc.) or stack.              !!
//
// ******************************************************************************************************************
class MemorySniffer {
  friend class PagedMemoryRegion;

private:
  typedef std::set<PagedMemoryRegion> PagedMemoryRegions;
  typedef std::map<PagedMemoryRegionPtr, PagedMemoryRegionHandle> RegionsPointersToHandles;
  PagedMemoryRegions _memRegions;
  RegionsPointersToHandles _regionPointersToHandles;
  std::recursive_mutex _regionsMutex;
  bool _originalSegvSignalFlag = false;
  bool _computeMode = false;
  static bool _isInstalled;
  static MemorySniffer* _instance;
#ifdef GITS_PLATFORM_WINDOWS
  static PVOID _exceptionHandler;
  static PVOID _continueHandler;
#endif

  MemorySniffer() {}
  PagedMemoryRegionHandle StoreRegionInternal(const PagedMemoryRegion region);
  std::set<PagedMemoryRegionHandle> GetPageRegionsInternal(const void* pagePtr);
  std::set<PagedMemoryRegionHandle> GetRangeRegionsInternal(const void* ptr, size_t len);

public:
  PagedMemoryRegionHandle CreateRegion(const void* ptr, size_t size);
  bool RemoveRegion(PagedMemoryRegionHandle handle);
  bool Protect(PagedMemoryRegionHandle handle);
  bool UnProtect(PagedMemoryRegionHandle handle);
  bool ReadProtect(PagedMemoryRegionHandle handle);
  bool WriteCallback(void* addr, bool writeIntention);
  bool WriteRange(void* addr, size_t size);
  static bool Install();
  static bool UnInstall();
  bool IsInstalled() const {
    return _isInstalled;
  }
  void SetOriginalSegvSignalFlag(const bool& value) {
    _originalSegvSignalFlag = value;
  };
  bool IsOriginalSegvSignalInitialized() const {
    return _originalSegvSignalFlag;
  }
  void SetComputeMode() {
    _computeMode = true;
  }
  static MemorySniffer& Get();
};
