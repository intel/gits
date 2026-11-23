// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "commandsAuto.h"

#include <mutex>
#include <unordered_map>

namespace gits {
namespace DirectX {

class DestructionCallbackService {
public:
  void PostSetPrivateDataInterface(ID3D12ObjectSetPrivateDataInterfaceCommand& c);
  void PostSetPrivateDataInterface(IDXGIObjectSetPrivateDataInterfaceCommand& c);
  void PreRegisterDestructionCallback(ID3DDestructionNotifierRegisterDestructionCallbackCommand& c);
  void PostRegisterDestructionCallback(
      ID3DDestructionNotifierRegisterDestructionCallbackCommand& c);

private:
  using Vtable = void**;
  using ReleaseFn = ULONG(STDMETHODCALLTYPE*)(IUnknown*);

  struct DestructionCallbackRegistration {
    PFN_DESTRUCTION_CALLBACK OriginalCallback{};
    void* OriginalData{};
  };

  struct VtableHook {
    Vtable HookedVtable{};
    ReleaseFn OriginalRelease{};
    uint64_t RefCount{};
  };

  static constexpr size_t MaxVtableSlots = 512;
  static constexpr size_t IUnknownReleaseIndex = 2;

  static bool IsExecutablePointer(void* pointer);
  static size_t CountVtableSlots(Vtable vtable);
  static VtableHook* GetVtableHook(Vtable originalVtable);
  static void InstallReleaseHook(IUnknown* object);
  static void UninstallReleaseHook(IUnknown* object);
  static ULONG STDMETHODCALLTYPE HookedRelease(IUnknown* self);

  static inline std::mutex s_ReleaseHookMutex;
  static inline std::unordered_map<IUnknown*, Vtable> s_ObjectOriginalVtables;
  static inline std::unordered_map<Vtable, VtableHook> s_VtableHooks;

  static void STDMETHODCALLTYPE DestructionCallback(void* pData);
};

} // namespace DirectX
} // namespace gits
