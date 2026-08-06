// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "destructionCallbackService.h"
#include "wrapperUtils.h"
#include "log.h"

#include <cstring>
#include <windows.h>

namespace gits {
namespace DirectX {

bool DestructionCallbackService::IsExecutablePointer(void* pointer) {
  if (!pointer) {
    return false;
  }
  MEMORY_BASIC_INFORMATION mbi{};
  if (VirtualQuery(pointer, &mbi, sizeof(mbi)) == 0) {
    return false;
  }
  if (mbi.State != MEM_COMMIT) {
    return false;
  }
  const DWORD protect = mbi.Protect & ~(PAGE_GUARD | PAGE_NOCACHE | PAGE_WRITECOMBINE);
  return protect == PAGE_EXECUTE || protect == PAGE_EXECUTE_READ ||
         protect == PAGE_EXECUTE_READWRITE || protect == PAGE_EXECUTE_WRITECOPY;
}

size_t DestructionCallbackService::CountVtableSlots(Vtable vtable) {
  constexpr size_t minSlots = IUnknownReleaseIndex + 1;

  MEMORY_BASIC_INFORMATION regionMbi{};
  if (VirtualQuery(vtable, &regionMbi, sizeof(regionMbi)) == 0) {
    LOG_ERROR << "VirtualQuery failed for vtable " << vtable << "; assuming " << minSlots
              << " vtable slots";
    return minSlots;
  }

  const auto* regionBegin = static_cast<const char*>(regionMbi.BaseAddress);
  const auto* regionEnd = regionBegin + regionMbi.RegionSize;
  const auto* tableBegin = reinterpret_cast<const char*>(vtable);
  if (tableBegin >= regionEnd) {
    LOG_ERROR << "Vtable " << vtable << " lies outside its memory region; assuming " << minSlots
              << " vtable slots";
    return minSlots;
  }

  const size_t slotsInRegion = static_cast<size_t>((regionEnd - tableBegin) / sizeof(void*));

  size_t count = 0;
  for (size_t slot = 0; slot < slotsInRegion; ++slot) {
    if (!IsExecutablePointer(vtable[slot])) {
      break;
    }
    count = slot + 1;
    if (count == MaxVtableSlots) {
      if (slotsInRegion > MaxVtableSlots) {
        LOG_ERROR << "Vtable slot scan for vtable " << vtable << " reached MaxVtableSlots ("
                  << MaxVtableSlots << "); copy may be incomplete";
      }
      break;
    }
  }

  if (count < minSlots) {
    LOG_ERROR << "Vtable " << vtable << " has " << count << " executable slot(s); assuming "
              << minSlots << " vtable slots";
    return minSlots;
  }
  return count;
}

DestructionCallbackService::VtableHook* DestructionCallbackService::GetVtableHook(
    Vtable originalVtable) {
  auto existing = s_VtableHooks.find(originalVtable);
  if (existing != s_VtableHooks.end()) {
    return &existing->second;
  }

  const size_t slotCount = CountVtableSlots(originalVtable);
  const size_t byteSize = slotCount * sizeof(void*);
  Vtable hookedVtable = static_cast<Vtable>(
      VirtualAlloc(nullptr, byteSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
  GITS_ASSERT(hookedVtable);

  std::memcpy(hookedVtable, originalVtable, byteSize);

  auto originalRelease = reinterpret_cast<ReleaseFn>(originalVtable[IUnknownReleaseIndex]);

  hookedVtable[IUnknownReleaseIndex] = reinterpret_cast<void*>(&HookedRelease);

  VtableHook vtableHook{};
  vtableHook.HookedVtable = hookedVtable;
  vtableHook.OriginalRelease = originalRelease;
  vtableHook.RefCount = 0;

  auto [it, inserted] = s_VtableHooks.emplace(originalVtable, vtableHook);
  GITS_ASSERT(inserted);
  return &it->second;
}

void DestructionCallbackService::InstallReleaseHook(IUnknown* object) {
  std::lock_guard<std::mutex> lock(s_ReleaseHookMutex);
  if (s_ObjectOriginalVtables.find(object) != s_ObjectOriginalVtables.end()) {
    return;
  }

  Vtable originalVtable = *reinterpret_cast<Vtable*>(object);
  VtableHook* vtableHook = GetVtableHook(originalVtable);
  if (!vtableHook) {
    return;
  }

  *reinterpret_cast<Vtable*>(object) = vtableHook->HookedVtable;

  GITS_ASSERT(s_ObjectOriginalVtables.emplace(object, originalVtable).second);
  ++vtableHook->RefCount;
}

void DestructionCallbackService::UninstallReleaseHook(IUnknown* object) {
  std::lock_guard<std::mutex> lock(s_ReleaseHookMutex);
  auto objectIt = s_ObjectOriginalVtables.find(object);
  if (objectIt == s_ObjectOriginalVtables.end()) {
    return;
  }

  Vtable originalVtable = objectIt->second;
  s_ObjectOriginalVtables.erase(objectIt);

  auto vtableIt = s_VtableHooks.find(originalVtable);
  GITS_ASSERT(vtableIt != s_VtableHooks.end());
  GITS_ASSERT(vtableIt->second.RefCount > 0);
  --vtableIt->second.RefCount;
  if (vtableIt->second.RefCount == 0) {
    VirtualFree(vtableIt->second.HookedVtable, 0, MEM_RELEASE);
    s_VtableHooks.erase(vtableIt);
  }
}

ULONG STDMETHODCALLTYPE DestructionCallbackService::HookedRelease(IUnknown* self) {
  ReleaseFn originalRelease{};
  {
    std::lock_guard<std::mutex> lock(s_ReleaseHookMutex);
    auto objectIt = s_ObjectOriginalVtables.find(self);
    GITS_ASSERT(objectIt != s_ObjectOriginalVtables.end(),
                "Release hook invoked for unregistered object");
    originalRelease = s_VtableHooks.at(objectIt->second).OriginalRelease;
  }

  self->AddRef();
  ULONG refCount = originalRelease(self);
  if (refCount == 1) {
    {
      NestedCaptureScope nestedCapture;
      refCount = originalRelease(self);
    }
    if (refCount == 0) {
      UninstallReleaseHook(self);
    }
  } else {
    refCount = originalRelease(self);
  }
  return refCount;
}

void DestructionCallbackService::PostSetPrivateDataInterface(
    ID3D12ObjectSetPrivateDataInterfaceCommand& c) {
  if (!c.m_pData.Value || FAILED(c.m_Result.Value)) {
    return;
  }
  InstallReleaseHook(const_cast<IUnknown*>(c.m_pData.Value));
}

void DestructionCallbackService::PostSetPrivateDataInterface(
    IDXGIObjectSetPrivateDataInterfaceCommand& c) {
  if (!c.m_pUnknown.Value || FAILED(c.m_Result.Value)) {
    return;
  }
  InstallReleaseHook(const_cast<IUnknown*>(c.m_pUnknown.Value));
}

void STDMETHODCALLTYPE DestructionCallbackService::DestructionCallback(void* pData) {
  auto* registration = static_cast<DestructionCallbackRegistration*>(pData);
  GITS_ASSERT(registration);
  GITS_ASSERT(registration->OriginalCallback);

  {
    NestedCaptureScope nestedCapture;
    registration->OriginalCallback(registration->OriginalData);
  }

  delete registration;
}

void DestructionCallbackService::PreRegisterDestructionCallback(
    ID3DDestructionNotifierRegisterDestructionCallbackCommand& c) {
  if (!c.m_callbackFn.Value) {
    return;
  }

  DestructionCallbackRegistration* registration = new DestructionCallbackRegistration{};
  registration->OriginalCallback = c.m_callbackFn.Value;
  registration->OriginalData = c.m_pData.Value;

  c.m_callbackFn.Value = DestructionCallback;
  c.m_pData.Value = registration;
}

void DestructionCallbackService::PostRegisterDestructionCallback(
    ID3DDestructionNotifierRegisterDestructionCallbackCommand& c) {
  auto* registration = static_cast<DestructionCallbackRegistration*>(c.m_pData.Value);
  if (!registration) {
    return;
  }

  if (FAILED(c.m_Result.Value)) {
    delete registration;
  }
}

} // namespace DirectX
} // namespace gits
