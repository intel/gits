// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "externalHandleService.h"

#include "log.h"

#include <algorithm>
#include <ranges>

#ifdef _WIN32
#include <windows.h>
#include <winternl.h>
#endif

namespace gits {
namespace stream {

namespace {

#ifdef _WIN32
// Available since Windows 10, so it is looked up instead of linked.
bool IsSameObject(void* first, void* second) {
  using CompareObjectHandlesFunc = BOOL(WINAPI*)(HANDLE, HANDLE);
  static const auto compareObjectHandles = reinterpret_cast<CompareObjectHandlesFunc>(
      GetProcAddress(GetModuleHandleW(L"kernelbase.dll"), "CompareObjectHandles"));
  if (compareObjectHandles == nullptr) {
    return first == second;
  }
  return compareObjectHandles(first, second) != FALSE;
}

void* DuplicateNtHandle(void* handle) {
  HANDLE duplicate = nullptr;
  if (!DuplicateHandle(GetCurrentProcess(), handle, GetCurrentProcess(), &duplicate, 0, FALSE,
                       DUPLICATE_SAME_ACCESS)) {
    return nullptr;
  }
  return duplicate;
}

void CloseNtHandle(void* handle) {
  CloseHandle(handle);
}

// Whether no other handle in any process refers to the object, so it can no longer be imported.
// NtQueryObject is looked up, as ntdll is not linked.
bool IsLastHandle(void* handle) {
  using NtQueryObjectFunc =
      NTSTATUS(NTAPI*)(HANDLE, OBJECT_INFORMATION_CLASS, PVOID, ULONG, PULONG);
  static const auto ntQueryObject = reinterpret_cast<NtQueryObjectFunc>(
      GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQueryObject"));
  if (ntQueryObject == nullptr) {
    return false;
  }
  PUBLIC_OBJECT_BASIC_INFORMATION info{};
  if (ntQueryObject(handle, ObjectBasicInformation, &info, sizeof(info), nullptr) < 0) {
    return false;
  }
  return info.HandleCount == 1;
}
#else
bool IsSameObject(void* first, void* second) {
  return first == second;
}

void* DuplicateNtHandle([[maybe_unused]] void* handle) {
  return nullptr;
}

void CloseNtHandle([[maybe_unused]] void* handle) {}

bool IsLastHandle([[maybe_unused]] void* handle) {
  return false;
}
#endif

// A KMT handle names its object only while the object exists. An NT handle keeps the payload
// alive after the object is destroyed, so its export is kept until it is closed.
template <typename Exports>
void RemoveExports(Exports& exports, uint64_t ownerKey) {
  auto removed = std::remove_if(exports.begin(), exports.end(), [&](auto& entry) {
    if (entry.OwnerKey != ownerKey) {
      return false;
    }
    entry.OwnerDestroyed = true;
    return !entry.NtHandle;
  });
  exports.erase(removed, exports.end());
}

} // namespace

ExternalHandleService& ExternalHandleService::Get() {
  static ExternalHandleService* service = new ExternalHandleService();
  return *service;
}

void ExternalHandleService::AddRecordedExport(uint64_t ownerKey,
                                              uint64_t exportKey,
                                              void* handle,
                                              bool ntHandle) {
  std::lock_guard lock(m_Mutex);
  PruneRecordedExports();
  // The application may close its handle while it still imports duplicates of it. One duplicate
  // keeps the object of all its exports, so that it is the only handle left once they are unused.
  void* storedHandle = handle;
  if (ntHandle) {
    const auto same = std::ranges::find_if(m_RecordedExports, [handle](const Export& entry) {
      return entry.NtHandle && IsSameObject(entry.Handle, handle);
    });
    storedHandle = same != m_RecordedExports.end() ? same->Handle : DuplicateNtHandle(handle);
    if (storedHandle == nullptr) {
      LOG_ERROR << "Command " << exportKey << " exported a handle that cannot be duplicated. "
                << "Imports of it cannot be recorded.";
      return;
    }
  }
  m_RecordedExports.push_back({ownerKey, exportKey, storedHandle, ntHandle});
}

void ExternalHandleService::RemoveRecordedExports(uint64_t ownerKey) {
  std::lock_guard lock(m_Mutex);
  RemoveExports(m_RecordedExports, ownerKey);
  PruneRecordedExports();
}

// Once the owners of all exports of an object are destroyed and the application closed its
// handles, only the duplicate is left, and nothing can import the exports anymore.
void ExternalHandleService::PruneRecordedExports() {
  const auto ownerAlive = [this](void* handle) {
    return std::ranges::any_of(m_RecordedExports, [handle](const Export& entry) {
      return entry.NtHandle && entry.Handle == handle && !entry.OwnerDestroyed;
    });
  };
  std::vector<void*> closed;
  const auto isClosed = [&closed](void* handle) {
    return std::ranges::find(closed, handle) != closed.end();
  };
  for (const Export& entry : m_RecordedExports) {
    if (entry.NtHandle && entry.OwnerDestroyed && !isClosed(entry.Handle) &&
        !ownerAlive(entry.Handle) && IsLastHandle(entry.Handle)) {
      CloseNtHandle(entry.Handle);
      closed.push_back(entry.Handle);
    }
  }
  std::erase_if(m_RecordedExports,
                [&](const Export& entry) { return entry.NtHandle && isClosed(entry.Handle); });
}

uint64_t ExternalHandleService::GetRecordedExportKey(void* handle, uint64_t importKey) const {
  std::lock_guard lock(m_Mutex);
  uint64_t exportKey = 0;
  for (const Export& entry : m_RecordedExports) {
    if (entry.ExportKey < importKey && entry.ExportKey > exportKey &&
        (entry.NtHandle ? IsSameObject(entry.Handle, handle) : entry.Handle == handle)) {
      exportKey = entry.ExportKey;
    }
  }
  return exportKey;
}

void ExternalHandleService::AddReplayedExport(uint64_t ownerKey,
                                              uint64_t exportKey,
                                              void* handle,
                                              bool ntHandle) {
  std::lock_guard lock(m_Mutex);
  m_ReplayedExports.push_back({ownerKey, exportKey, handle, ntHandle});
}

void ExternalHandleService::RemoveReplayedExports(uint64_t ownerKey) {
  std::lock_guard lock(m_Mutex);
  RemoveExports(m_ReplayedExports, ownerKey);
}

void ExternalHandleService::CloseReplayedExports() {
  std::lock_guard lock(m_Mutex);
  for (const auto& entry : m_ReplayedExports) {
    if (entry.NtHandle) {
      CloseNtHandle(entry.Handle);
    }
  }
  m_ReplayedExports.clear();
}

void* ExternalHandleService::GetReplayedExport(uint64_t exportKey) const {
  std::lock_guard lock(m_Mutex);
  for (const auto& entry : m_ReplayedExports) {
    if (entry.ExportKey == exportKey) {
      return entry.Handle;
    }
  }
  return nullptr;
}

} // namespace stream
} // namespace gits
