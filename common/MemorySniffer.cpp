// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

// MemorySniffer.cpp : Defines the entry point for the console application.
//

#include "platform.h"
#ifdef GITS_PLATFORM_WINDOWS
#include <windows.h>
#include "tools_windows.h"
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <csignal>
#include <unistd.h>
#include <cstdio>
#endif

#include "MemorySniffer.h"
#include "gits.h"
#include <cinttypes>
#include <string>
#include <iostream>
#include <algorithm>
#include "log.h"
#include "exception.h"

//
//  Following macro and associated template function
//  are used to workaround issue found in STL that ships with g++4.3.3 -
//  set iterators are made 'const_iterators' under the hood (probably
//  to prevent changes to objects that would affect their ordering,
//  this also prevents changes that do not affect ordering of set elements
//  As a workaround i provide GCC433WA_0 macro that takes a set iterator
//  dereferences it, casts away const from resulting lvalue and returns
//  its address (there may be potential issues here, as type of expression
//  change to 'pointer to T' but this looks like the easiest solution
//  On Windows this is a noop
//
//  Apparently, it was clarified in next C++ standard that above behavior
//  is intended one, so we are in fact illegal, the workaround will stay
//  untill all clieant are refactored to use std::map instead of std::set
//
#ifndef GCC433WA_0
template <class T>
T& castaway_const(const T& x) {
  return const_cast<T&>(x);
}
#define GCC433WA_0(iter) (&castaway_const(*iter))
#endif

// ******************************************************************************************************************
//
// GetVirtualMemoryPageSize - Returns system memory page size.
//
// ******************************************************************************************************************
unsigned GetVirtualMemoryPageSize() {
#ifdef GITS_PLATFORM_WINDOWS
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  return si.dwPageSize;
#else
  return getpagesize();
#endif
}

// ******************************************************************************************************************
//
// GetPage - Returns address to the page where passed address is located.
//
// ******************************************************************************************************************
void* GetPage(void* addr) {
  return (void*)((uint64_t)addr - (uint64_t)addr % GetVirtualMemoryPageSize());
}
const void* GetPage(const void* addr) {
  return (const void*)((uint64_t)addr - (uint64_t)addr % GetVirtualMemoryPageSize());
}

// ******************************************************************************************************************
//
// TouchPageInternal - Saves page address in the list of accessed pages of memory region.
//
// ******************************************************************************************************************
void PagedMemoryRegion::TouchPageInternal(const void* ptr) {
  assert(((uint64_t)ptr % GetVirtualMemoryPageSize()) == 0);
  if (_protected) {
    _touchedPages.insert(ptr);
  }
}
const PagedMemoryRegion::TouchedPages PagedMemoryRegion::GetTouchedPages() const {
  std::unique_lock<std::recursive_mutex> lock(MemorySniffer::Get()._regionsMutex);
  return _touchedPages;
}
const PagedMemoryRegion::TouchedPages PagedMemoryRegion::GetTouchedPagesAndReset() {
  std::unique_lock<std::recursive_mutex> lock(MemorySniffer::Get()._regionsMutex);
  PagedMemoryRegion::TouchedPages _copy;
  _copy.swap(_touchedPages);
  return _copy;
}

void PagedMemoryRegion::Reset() {
  std::unique_lock<std::recursive_mutex> lock(MemorySniffer::Get()._regionsMutex);
  _touchedPages.clear();
}

// ******************************************************************************************************************
//
// SetPagesProtection - Changes memory protection settings for pages that overlaps passed memory range.
// Size parameter is in bytes and its default value is 1.
//
// ******************************************************************************************************************
bool SetPagesProtection(PageMemoryProtection access, void* ptr, size_t size) {
  size_t pageSize = GetVirtualMemoryPageSize();
  char* rangeBegin = (char*)ptr;
  char* rangeBeginPage = (char*)GetPage(rangeBegin);
  char* rangeEnd = (char*)ptr + size;
  char* rangeEndPage =
      (char*)GetPage(rangeEnd) + (((uint64_t)rangeEnd % pageSize > 0) ? pageSize : 0);
  size_t rangeSizeWholePages = (size_t)((uint64_t)rangeEndPage - (uint64_t)rangeBeginPage);

#ifdef GITS_PLATFORM_WINDOWS
  unsigned winAccess;
  switch (access) {
  case READ_ONLY:
    winAccess = PAGE_READONLY;
    break;
  case READ_WRITE:
    winAccess = PAGE_READWRITE;
    break;
  case WRITE_ONLY:
    winAccess = PAGE_WRITECOPY;
    break;
  case NONE:
    winAccess = PAGE_NOACCESS;
    break;
  default:
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }
  DWORD prevProtection;

  if (VirtualProtect(rangeBeginPage, rangeSizeWholePages, winAccess, &prevProtection) == 0) {
    std::string errorStr = gits::Win32ErrorToString(GetLastError());
    std::string winAccessStr;
    switch (winAccess) {
    case PAGE_READONLY:
      winAccessStr = "PAGE_READONLY";
      break;
    case PAGE_READWRITE:
      winAccessStr = "PAGE_EXECUTE_READWRITE";
      break;
    case PAGE_WRITECOPY:
      winAccessStr = "PAGE_WRITECOPY";
      break;
    case PAGE_NOACCESS:
      winAccessStr = "PAGE_NOACCESS";
      break;
    default:
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
    Log(WARN) << "VirtualProtect API failed on changing memory ptr: " << ptr << " of size: " << size
              << " to " << winAccessStr << " with error: " << errorStr << std::endl;
    return false;
  }
  return true;
#else
  unsigned unixAccess;
  switch (access) {
  case READ_ONLY:
    unixAccess = PROT_READ;
    break;
  case READ_WRITE:
    unixAccess = PROT_READ | PROT_WRITE;
    break;
  case WRITE_ONLY:
    unixAccess = PROT_WRITE;
    break;
  case NONE:
    unixAccess = PROT_NONE;
    break;
  default:
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  errno = 0;
  if (mprotect(rangeBeginPage, rangeSizeWholePages, unixAccess) == -1) {
    std::string unixAccessStr;
    switch (unixAccess) {
    case PROT_READ:
      unixAccessStr = "PROT_READ";
      break;
    case (PROT_READ | PROT_WRITE):
      unixAccessStr = "PROT_READ | PROT_WRITE";
      break;
    case PROT_WRITE:
      unixAccessStr = "PROT_WRITE";
      break;
    case PROT_NONE:
      unixAccessStr = "PROT_NONE";
      break;
    default:
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
    Log(WARN) << "mprotect API failed on changing memory ptr: " << ptr << " of size: " << size
              << " to " << unixAccessStr << " errno: " << errno << std::endl;
    return false;
  }
  return true;
#endif
}

//**************************************************************************************************
//
// ExternalMemoryRegion::GetTouchedPagesAndReset - returns set of memory regions which were
// modified since the last check or reset.
//
//**************************************************************************************************

std::vector<std::pair<void*, uint32_t>> ExternalMemoryRegion::GetTouchedPagesAndReset(
    void* ptr, uint32_t size) {
#ifdef GITS_PLATFORM_WINDOWS
  auto pageSize = GetVirtualMemoryPageSize();
  ULONG_PTR pageCount = (size / pageSize) + ((size % pageSize > 0) ? 1 : 0);
  std::vector<void*> touchedPages(pageCount);
  DWORD granularity = 0;
  if (!GetWriteWatch(WRITE_WATCH_FLAG_RESET, ptr, size, touchedPages.data(), &pageCount,
                     &granularity) &&
      (pageCount > 0)) {

    std::vector<std::pair<void*, uint32_t>> touchedMemory;
    void* basePtr = touchedPages[0];
    uint32_t length = pageSize;

    for (size_t i = 1; i < pageCount; ++i) {
      if (touchedPages[i] == ((char*)basePtr + length)) {
        length += pageSize;
      } else {
        touchedMemory.push_back(std::make_pair(basePtr, length));
        basePtr = touchedPages[i];
        length = pageSize;
      }
    }
    if (length > 0) {
      touchedMemory.push_back(std::make_pair(basePtr, length));
    }
    // Adjust the base ptr of the first entry
    auto& firstEntry = touchedMemory.front();
    if (firstEntry.first < ptr) {
      auto diff = (char*)ptr - (char*)firstEntry.first;
      firstEntry.first = ptr;
      firstEntry.second -= diff;
    }

    // Trim the length of the last entry
    auto& lastEntry = touchedMemory.back();
    if (((char*)lastEntry.first + lastEntry.second) > ((char*)ptr + size)) {
      lastEntry.second = (char*)ptr + size - (char*)lastEntry.first;
    }
    return touchedMemory;
  }
#endif

  return std::vector<std::pair<void*, uint32_t>>();
}

//**************************************************************************************************
//
// ExternalMemoryRegion::ResetTouchedPages - discards information if the memory region was
// touched/modified.
//
//**************************************************************************************************

void ExternalMemoryRegion::ResetTouchedPages(void* ptr, uint32_t size) {
#ifdef GITS_PLATFORM_WINDOWS
  ResetWriteWatch(ptr, size);
#endif
}

//**************************************************************************************************
//
// MemorySniffer::StoreRegionInternal - Internal method to store PagedMemoryRegion object
// along with it's handle and pointer.
//
//**************************************************************************************************
PagedMemoryRegionHandle MemorySniffer::StoreRegionInternal(const PagedMemoryRegion region) {

  //Store region
  auto setInsertResult = _memRegions.insert(region);

  //Store region, pointer to region and handle to region
  //_ptr argument of PagedMemoryRegion class used for map elements comparison is const so we
  // can safely cast this class to non const there.
  PagedMemoryRegionPtr regionPtr = const_cast<PagedMemoryRegionPtr>(&(*setInsertResult.first));
  auto mapInsertResult =
      _regionPointersToHandles.insert(RegionsPointersToHandles::value_type(regionPtr, 0));
  auto iter = mapInsertResult.first;
  //Safe const_cast - all operations on this map are performed internally by this class and are safe in case of key manipulations
  iter->second = const_cast<PagedMemoryRegionHandle>(&iter->first);
  return iter->second;
}

//**************************************************************************************************
//
// MemorySniffer::GetRangeRegionsInternal - returns set of regions that overlaps passed addresses range.
//
//**************************************************************************************************
std::set<PagedMemoryRegionHandle> MemorySniffer::GetRangeRegionsInternal(const void* ptr,
                                                                         size_t len) {
  void* rangeBegin = (void*)ptr;
  void* rangeEnd = (void*)((uint64_t)ptr + len);

  std::set<PagedMemoryRegionHandle> output;

  //Check regions started on this page
  MemorySniffer::PagedMemoryRegions::const_iterator iter =
      _memRegions.lower_bound(PagedMemoryRegion(rangeBegin, 0));
  auto iterCopy = iter;
  while (iter != _memRegions.end() && iter->BeginAddress() < rangeEnd) {
    PagedMemoryRegionPtr rangePtr = GCC433WA_0(&(*iter));
    assert(_regionPointersToHandles.find(rangePtr) != _regionPointersToHandles.end());
    output.insert(_regionPointersToHandles[rangePtr]);
    iter++;
  }
  //Check region ended on this page
  iter = iterCopy;
  if (iter != _memRegions.begin()) {
    --iter;
    if (iter->EndAddress() > rangeBegin) {
      PagedMemoryRegionPtr rangePtr = GCC433WA_0(&(*iter));
      assert(_regionPointersToHandles.find(rangePtr) != _regionPointersToHandles.end());
      output.insert(_regionPointersToHandles[rangePtr]);
    }
  }
  return output;
}

//**************************************************************************************************
//
// MemorySniffer::GetPageRegionsInternal - returns set of regions that overlaps passed page area.
//
//**************************************************************************************************
std::set<PagedMemoryRegionHandle> MemorySniffer::GetPageRegionsInternal(const void* pagePtr) {
  if (((uint64_t)pagePtr % GetVirtualMemoryPageSize()) != 0) {
    throw std::runtime_error(EXCEPTION_MESSAGE);
  }

  return GetRangeRegionsInternal(pagePtr, GetVirtualMemoryPageSize());
}

//**************************************************************************************************
//
// MemorySniffer::CreateRegionInternal - Creates a described region, stores it and returns handle
// to the stored object or, if asked region overlaps existing one returns NULL.
//
//**************************************************************************************************
PagedMemoryRegionHandle MemorySniffer::CreateRegion(const void* ptr, size_t size) {
  std::unique_lock<std::recursive_mutex> lock(_regionsMutex);

  //Validate new region
  PagedMemoryRegion newRegion(ptr, size);
  if (_memRegions.find(newRegion) != _memRegions.end()) {
    //Region already exists
    return nullptr;
  }

  auto upperIter = _memRegions.upper_bound(newRegion);
  if (upperIter != _memRegions.end() && upperIter->BeginAddress() < newRegion.EndAddress()) {
    //Region overlaps upper region
    return nullptr;
  }

  if (upperIter != _memRegions.begin()) {
    auto lowerIter = --upperIter;
    if (newRegion.BeginAddress() < lowerIter->EndAddress()) {
      //Region overlaps lower region
      return nullptr;
    }
  }

  //Store region
  return StoreRegionInternal(std::move(newRegion));
}

//**************************************************************************************************
//
// MemorySniffer::RemoveRegion - removes region pointed by handle and all related data
//
//**************************************************************************************************
bool MemorySniffer::RemoveRegion(PagedMemoryRegionHandle handle) {
  std::unique_lock<std::recursive_mutex> lock(_regionsMutex);
  assert(handle);
  if (*handle == nullptr) {
    return false;
  }
  if (_memRegions.find(**handle) == _memRegions.end()) {
    return false;
  }

  bool unprotectResult = UnProtect(handle);
  if (unprotectResult == false) {
    Log(WARN) << "Restoring memory page's access rights FAILED!!!" << std::endl;
  }

  _memRegions.erase(**handle);
  _regionPointersToHandles.erase(*handle);

  //Set pointer to NULL to mark handle as empty
  *handle = nullptr;
  return true;
}

//**************************************************************************************************
//
// MemorySniffer::Protect - Changes memory range pages access rights to read only.
//
//**************************************************************************************************
bool MemorySniffer::Protect(PagedMemoryRegionHandle handle) {
  std::unique_lock<std::recursive_mutex> lock(_regionsMutex);

  //Validate region
  assert(handle);
  assert(*handle);
  PagedMemoryRegion& region = **handle;
  if (_memRegions.find(region) == _memRegions.end()) {
    return false;
  }

  bool result = SetPagesProtection(READ_ONLY, (void*)region.BeginPage(), region.SizeOfPages());
  if (result == true) {
    region._protected = true;
  }
  return result;
}

//**************************************************************************************************
//
// MemorySniffer::ReadProtect - Changes memory range pages access rights to write only.
//
//**************************************************************************************************
bool MemorySniffer::ReadProtect(PagedMemoryRegionHandle handle) {
  std::unique_lock<std::recursive_mutex> lock(_regionsMutex);

  //Validate region
  assert(handle);
  assert(*handle);
  PagedMemoryRegion& region = **handle;
  if (_memRegions.find(region) == _memRegions.end()) {
    return false;
  }

  bool result = SetPagesProtection(WRITE_ONLY, (void*)region.BeginPage(), region.SizeOfPages());
  if (result == true) {
    region._protected = true;
  }
  return result;
}

//**************************************************************************************************
//
// MemorySniffer::UnProtect - Changes memory range pages access rights to read write.
// It skips border pages overlapped by other ranges also.
//
//**************************************************************************************************
bool MemorySniffer::UnProtect(PagedMemoryRegionHandle handle) {
  std::unique_lock<std::recursive_mutex> lock(_regionsMutex);

  //Validate region
  assert(handle);
  PagedMemoryRegion& region = **handle;
  if (_memRegions.find(region) == _memRegions.end()) {
    return false;
  }

  //Skip border pages if they are used by other regions too.
  void* memPageBeginPtr = const_cast<void*>(region.BeginPage());
  void* memPageEndPtr = const_cast<void*>(region.EndPage());
  size_t size = region.SizeOfPages();
  if (GetPageRegionsInternal(memPageBeginPtr).size() > 1) {
    if (memPageBeginPtr < memPageEndPtr) {
      memPageBeginPtr = (void*)((uint64_t)memPageBeginPtr + GetVirtualMemoryPageSize());
    }
    if (size > 0) {
      size = size - GetVirtualMemoryPageSize();
    }
  }

  if (GetPageRegionsInternal(memPageEndPtr).size() > 1 && memPageBeginPtr != memPageEndPtr) {
    if (memPageEndPtr > memPageBeginPtr) {
      memPageBeginPtr = (void*)((uint64_t)memPageEndPtr - GetVirtualMemoryPageSize());
    }
    if (size > 0) {
      size = size - GetVirtualMemoryPageSize();
    }
  }
  if (size > 0) {
    bool result = SetPagesProtection(READ_WRITE, memPageBeginPtr, size);
    if (result == true) {
      region._protected = false;
    }
    return result;
  } else {
    region._protected = false;
    return true;
  }
}

//**************************************************************************************************
//
// MemorySniffer::WriteCallback - Method that should be registered to be called on memory access violation.
// If memory page region overlaps any regions it informs them about, unprotects page and returns true.
// Otherwise it returns false.
//
//**************************************************************************************************
bool MemorySniffer::WriteCallback(void* addr) {
  std::unique_lock<std::recursive_mutex> lock(_regionsMutex);
  void* pageAddr = GetPage(addr);
  auto pageRegionHandles = GetPageRegionsInternal(pageAddr);
  if (pageRegionHandles.size() == 0) {
    return false;
  }

  auto unveilWholeRegion = false;
  if (_unveilWholeRegion) {
    const auto& computeIFace = gits::CGits::Instance().apis.IfaceCompute();
    if (computeIFace.VerifyAllocation(addr)) {
      unveilWholeRegion = _unveilWholeRegion;
    }
  }
  if (!unveilWholeRegion) {
    SetPagesProtection(READ_WRITE, addr);
  }
  for (auto& handle : pageRegionHandles) {
    assert(handle);
    assert(*handle);
    bool result = false;
    auto& region = **handle;
    region.TouchPageInternal(pageAddr);
    if (unveilWholeRegion) {
      result =
          SetPagesProtection(READ_WRITE, const_cast<void*>(region.BeginAddress()), region.Size());
    }
    if (result) {
      region._protected = false;
    }
  }
  return true;
}

//**************************************************************************************************
//
// MemorySniffer::WriteRange - If memory range overlaps any registered regions it informs them about it and
// unprotects overlapped pages and returns true.
// Otherwise it returns false.
//
//**************************************************************************************************
bool MemorySniffer::WriteRange(void* addr, size_t size) {
  std::unique_lock<std::recursive_mutex> lock(_regionsMutex);
  size_t pageSize = GetVirtualMemoryPageSize();
  char* rangeBegin = (char*)addr;
  char* rangeBeginPage = (char*)GetPage(rangeBegin);
  char* rangeEnd = (char*)addr + size;
  char* rangeEndPage =
      (char*)GetPage(rangeEnd) + (((uint64_t)rangeEnd % pageSize > 0) ? pageSize : 0);
  size_t rangeSizeWholePages = (size_t)((uint64_t)rangeEndPage - (uint64_t)rangeBeginPage);
  auto touchedMemRegionsHandles = GetRangeRegionsInternal(rangeBeginPage, rangeSizeWholePages);
  bool result = false;
  //Unprotect range
  if (touchedMemRegionsHandles.size() > 0) {
    SetPagesProtection(READ_WRITE, addr, size);
    result = true;
  }
  //Mark overlapping regions as touched
  for (auto regionHandle : touchedMemRegionsHandles) {
    char* regionBegin = (char*)(**regionHandle).BeginAddress();
    char* regionBeginPage = (char*)GetPage(regionBegin);
    char* regionEnd = (char*)(**regionHandle).EndAddress();
    char* regionEndPage =
        (char*)GetPage(regionEnd) + (((uint64_t)regionEnd % pageSize > 0) ? pageSize : 0);
    char* touchedBeginPage = std::max(regionBeginPage, rangeBeginPage);
    char* touchedEndPage = std::min(regionEndPage, rangeEndPage);
    char* pagePtr = touchedBeginPage;
    while (pagePtr < touchedEndPage) {
      (**regionHandle).TouchPageInternal(pagePtr);
      pagePtr = pagePtr + GetVirtualMemoryPageSize();
    }
  }
  return result;
}

#ifdef GITS_PLATFORM_WINDOWS
LONG WINAPI MemorySnifferExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo) {
  if (ExceptionInfo != NULL) {
    PEXCEPTION_RECORD pExcptRec = ExceptionInfo->ExceptionRecord;
    if (pExcptRec->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
      // Need to check if this exception is caused by a write operation
      // 1 - the constant for write operation
      if (pExcptRec->NumberParameters >= 2 && pExcptRec->ExceptionInformation[0] == 1) {
        void* address = (void*)pExcptRec->ExceptionInformation[1];
        if (MemorySniffer::Get().WriteCallback(address)) {
          return EXCEPTION_CONTINUE_EXECUTION;
        }
      }
    }
  }

  // Pass external exception to the next handler.
  return EXCEPTION_CONTINUE_SEARCH;
}

#else
static void MemorySnifferSignalHandler(int sig, siginfo_t* si, void* unused);

namespace {
std::mutex signalMutex;
bool originalSegvInitialized = false, originalBusInitialized = false;
struct sigaction originalSigactionSegv = {}, originalSigactionBus = {};
void CaptureOriginalSigBusHandler() {
  std::unique_lock<std::mutex> lock(signalMutex);
  struct sigaction saBus = {};
  errno = 0;
  if (sigaction(SIGBUS, nullptr, &saBus) == -1) {
    Log(WARN) << "obtaining previous sigaction for SIGBUS failed. Errno: " << errno;
    originalBusInitialized = false;
    return;
  }
  if ((saBus.sa_flags & SA_SIGINFO) != 0 && saBus.sa_sigaction != MemorySnifferSignalHandler &&
      saBus.sa_sigaction != nullptr) {
    originalSigactionBus = saBus;
    originalBusInitialized = true;
  }
}
void CaptureOriginalSigSegvHandler() {
  std::unique_lock<std::mutex> lock(signalMutex);
  struct sigaction saSegv = {};
  errno = 0;
  if (sigaction(SIGSEGV, nullptr, &saSegv) == -1) {
    Log(WARN) << "obtaining previous sigaction for SIGSEGV failed. Errno: " << errno;
    originalSegvInitialized = false;
    return;
  }
  if ((saSegv.sa_flags & SA_SIGINFO) != 0 && saSegv.sa_sigaction != MemorySnifferSignalHandler &&
      saSegv.sa_sigaction != nullptr) {
    originalSigactionSegv = saSegv;
    originalSegvInitialized = true;
  }
  MemorySniffer::Get().SetOriginalSegvSignalFlag(originalSegvInitialized);
}

void CallOriginalSignalHandler(int sig, siginfo_t* si, void* unused) {
  std::unique_lock<std::mutex> lock(signalMutex);
  struct sigaction* act = nullptr;
  if (sig == SIGSEGV && originalSegvInitialized) {
    act = &originalSigactionSegv;
  } else if (sig == SIGBUS && originalBusInitialized) {
    act = &originalSigactionBus;
  } else {
    return;
  }

  if (act && act->sa_flags & SA_SIGINFO) {
    //act->sa_sigaction(sig, si, unused);
    auto funcall = act->sa_sigaction;
    lock.unlock();
    funcall(sig, si, unused);
    MemorySniffer::Install();
  } else if (act && act->sa_handler != SIG_DFL && act->sa_handler != SIG_IGN) {
    //act->sa_handler(si->si_signo);
    auto funcall = act->sa_handler;
    lock.unlock();
    funcall(si->si_signo);
    MemorySniffer::Install();
  }
}
} // namespace

static void MemorySnifferSignalHandler(int sig, siginfo_t* si, void* unused) {
  // Original signal handler must be called first
  CallOriginalSignalHandler(sig, si, unused);
  if (!MemorySniffer::Get().WriteCallback(si->si_addr)) {
    Log(WARN) << "MemorySnifferSignalHandler caught an unhandled signal: " << sig
              << " errno: " << si->si_errno << " at address: " << si->si_addr;
    exit(1);
  }
}
#endif

//**************************************************************************************************
//
// MemorySniffer::Install - Static method that installs WriteCallback method in the system.
//
//**************************************************************************************************
bool MemorySniffer::Install() {
#ifdef GITS_PLATFORM_WINDOWS
  auto exceptionFilterHandle = AddVectoredExceptionHandler(1, MemorySnifferExceptionFilter);
  if (exceptionFilterHandle == NULL) {
    return false;
  }
  return true;
#else
  struct sigaction sa = {};
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = &MemorySnifferSignalHandler;
  if (!originalSegvInitialized) {
    CaptureOriginalSigSegvHandler();
  }
  if (!originalBusInitialized) {
    CaptureOriginalSigBusHandler();
  }

  errno = 0;
  if (sigaction(SIGSEGV, &sa, nullptr) == -1) {
    Log(WARN) << "sigaction setup SIGSEGV failed. Errno: " << errno;
    return false;
  }
  if (sigaction(SIGBUS, &sa, nullptr) == -1) {
    Log(WARN) << "sigaction setup SIGBUS failed. Errno: " << errno;
    return false;
  }
  _isInstalled = true;
  return true;
#endif
}

bool MemorySniffer::UnInstall() {
#ifdef GITS_PLATFORM_WINDOWS
  auto exceptionFilterHandle = RemoveVectoredExceptionHandler(MemorySnifferExceptionFilter);
  if (exceptionFilterHandle == NULL) {
    return false;
  }
  return true;
#else
  struct sigaction sa = {};
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = nullptr;
  errno = 0;
  originalSegvInitialized = false;
  originalBusInitialized = false;
  if (sigaction(SIGSEGV, &sa, nullptr) == -1) {
    Log(WARN) << "sigaction setup SIGSEGV failed. Errno: " << errno;
    return false;
  }
  if (sigaction(SIGBUS, &sa, nullptr) == -1) {
    Log(WARN) << "sigaction setup SIGBUS failed. Errno: " << errno;
    return false;
  }
  _isInstalled = false;
  return true;
#endif
}

bool MemorySniffer::_isInstalled = false;

MemorySniffer* MemorySniffer::_instance;

MemorySniffer& MemorySniffer::Get() {
  if (_instance == nullptr) {
    _instance = new MemorySniffer;
  }
  return *_instance;
}
