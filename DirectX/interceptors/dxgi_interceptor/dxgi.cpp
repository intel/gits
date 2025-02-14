// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "d3d12RecorderInterface.h"
#include "gitsLoader.h"

#include <string>
#include <filesystem>
#include <fstream>
#include <cassert>
#include <detours.h>

gits::DirectX::DXGIDispatchTable g_dispatchTableSystem;
gits::DirectX::DXGIDispatchTable g_dispatchTableWrapper;

using ApplyCompatResolutionQuirkingPtr = HRESULT(WINAPI*)();
using CompatStringPtr = HRESULT(WINAPI*)();
using CompatValuePtr = HRESULT(WINAPI*)();
using DXGIDumpJournalPtr = HRESULT(WINAPI*)();
using PIXBeginCapturePtr = HRESULT(WINAPI*)();
using PIXEndCapturePtr = HRESULT(WINAPI*)();
using PIXGetCaptureStatePtr = HRESULT(WINAPI*)();
using SetAppCompatStringPointerPtr = HRESULT(WINAPI*)();
using UpdateHMDEmulationStatusPtr = HRESULT(WINAPI*)();
using DXGID3D10CreateDevicePtr = HRESULT(WINAPI*)();
using DXGID3D10CreateLayeredDevicePtr = HRESULT(WINAPI*)();
using DXGID3D10GetLayeredDeviceSizePtr = HRESULT(WINAPI*)();
using DXGID3D10RegisterLayersPtr = HRESULT(WINAPI*)();
using DXGIReportAdapterConfigurationPtr = HRESULT(WINAPI*)();

struct DXGIDispatchTableForwarded {
  decltype(DXGIDisableVBlankVirtualization)* DXGIDisableVBlankVirtualization{nullptr};
  decltype(DXGIGetDebugInterface)* DXGIGetDebugInterface{nullptr};
  ApplyCompatResolutionQuirkingPtr ApplyCompatResolutionQuirking{};
  CompatStringPtr CompatString{};
  CompatValuePtr CompatValue{};
  DXGIDumpJournalPtr DXGIDumpJournal{};
  PIXBeginCapturePtr PIXBeginCapture{};
  PIXEndCapturePtr PIXEndCapture{};
  PIXGetCaptureStatePtr PIXGetCaptureState{};
  SetAppCompatStringPointerPtr SetAppCompatStringPointer{};
  UpdateHMDEmulationStatusPtr UpdateHMDEmulationStatus{};
  DXGID3D10CreateDevicePtr DXGID3D10CreateDevice{};
  DXGID3D10CreateLayeredDevicePtr DXGID3D10CreateLayeredDevice{};
  DXGID3D10GetLayeredDeviceSizePtr DXGID3D10GetLayeredDeviceSize{};
  DXGID3D10RegisterLayersPtr DXGID3D10RegisterLayers{};
  DXGIReportAdapterConfigurationPtr DXGIReportAdapterConfiguration{};
};
DXGIDispatchTableForwarded g_dispatchTableForwarded;

HMODULE g_systemDll{0};
std::unique_ptr<gits::CGitsLoader> g_gitsLoader;

void initializeDispatchTable() {
  static bool s_initialized = false;
  if (s_initialized) {
    return;
  }
  s_initialized = true;

  g_systemDll = LoadLibrary("C:\\Windows\\System32\\dxgi.dll");
  assert(g_systemDll);

  g_dispatchTableSystem.CreateDXGIFactory = reinterpret_cast<decltype(CreateDXGIFactory)*>(
      GetProcAddress(g_systemDll, "CreateDXGIFactory"));

  g_dispatchTableSystem.CreateDXGIFactory1 = reinterpret_cast<decltype(CreateDXGIFactory1)*>(
      GetProcAddress(g_systemDll, "CreateDXGIFactory1"));

  g_dispatchTableSystem.CreateDXGIFactory2 = reinterpret_cast<decltype(CreateDXGIFactory2)*>(
      GetProcAddress(g_systemDll, "CreateDXGIFactory2"));

  g_dispatchTableSystem.DXGIDeclareAdapterRemovalSupport =
      reinterpret_cast<decltype(DXGIDeclareAdapterRemovalSupport)*>(
          GetProcAddress(g_systemDll, "DXGIDeclareAdapterRemovalSupport"));

  g_dispatchTableSystem.DXGIGetDebugInterface1 =
      reinterpret_cast<decltype(DXGIGetDebugInterface1)*>(
          GetProcAddress(g_systemDll, "DXGIGetDebugInterface1"));
}

void hookDxgi() {
  static bool s_initialized = false;
  if (s_initialized) {
    return;
  }
  s_initialized = true;

  initializeDispatchTable();

  LONG ret = DetourTransactionBegin();
  assert(ret == NO_ERROR);

  ret = DetourUpdateThread(GetCurrentThread());
  assert(ret == NO_ERROR);

  ret = DetourAttach(&g_dispatchTableSystem.CreateDXGIFactory, CreateDXGIFactory);
  assert(ret == NO_ERROR);
  ret = DetourAttach(&g_dispatchTableSystem.CreateDXGIFactory1, CreateDXGIFactory1);
  assert(ret == NO_ERROR);
  ret = DetourAttach(&g_dispatchTableSystem.CreateDXGIFactory2, CreateDXGIFactory2);
  assert(ret == NO_ERROR);
  ret = DetourAttach(&g_dispatchTableSystem.DXGIDeclareAdapterRemovalSupport,
                     DXGIDeclareAdapterRemovalSupport);
  assert(ret == NO_ERROR);
  ret = DetourAttach(&g_dispatchTableSystem.DXGIGetDebugInterface1, DXGIGetDebugInterface1);
  assert(ret == NO_ERROR);

  ret = DetourTransactionCommit();
  assert(ret == NO_ERROR);
}

void initialize() {
  static bool s_initialized = false;
  if (s_initialized) {
    return;
  }
  s_initialized = true;

  g_gitsLoader = std::make_unique<gits::CGitsLoader>("GITSRecorderD3D12");

  initializeDispatchTable();

  g_dispatchTableForwarded.DXGIDisableVBlankVirtualization =
      reinterpret_cast<decltype(DXGIDisableVBlankVirtualization)*>(
          GetProcAddress(g_systemDll, "DXGIDisableVBlankVirtualization"));
  g_dispatchTableForwarded.DXGIGetDebugInterface =
      +reinterpret_cast<decltype(DXGIGetDebugInterface)*>(
          +GetProcAddress(g_systemDll, "DXGIGetDebugInterface"));
  g_dispatchTableForwarded.ApplyCompatResolutionQuirking =
      reinterpret_cast<ApplyCompatResolutionQuirkingPtr>(
          GetProcAddress(g_systemDll, "ApplyCompatResolutionQuirking"));
  g_dispatchTableForwarded.CompatString =
      reinterpret_cast<CompatStringPtr>(GetProcAddress(g_systemDll, "CompatString"));
  g_dispatchTableForwarded.CompatValue =
      reinterpret_cast<CompatValuePtr>(GetProcAddress(g_systemDll, "CompatValue"));
  g_dispatchTableForwarded.DXGIDumpJournal =
      reinterpret_cast<DXGIDumpJournalPtr>(GetProcAddress(g_systemDll, "DXGIDumpJournal"));
  g_dispatchTableForwarded.PIXBeginCapture =
      reinterpret_cast<PIXBeginCapturePtr>(GetProcAddress(g_systemDll, "PIXBeginCapture"));
  g_dispatchTableForwarded.PIXEndCapture =
      reinterpret_cast<PIXEndCapturePtr>(GetProcAddress(g_systemDll, "PIXEndCapture"));
  g_dispatchTableForwarded.PIXGetCaptureState =
      reinterpret_cast<PIXGetCaptureStatePtr>(GetProcAddress(g_systemDll, "PIXGetCaptureState"));
  g_dispatchTableForwarded.SetAppCompatStringPointer =
      reinterpret_cast<SetAppCompatStringPointerPtr>(
          GetProcAddress(g_systemDll, "SetAppCompatStringPointer"));
  g_dispatchTableForwarded.UpdateHMDEmulationStatus = reinterpret_cast<UpdateHMDEmulationStatusPtr>(
      GetProcAddress(g_systemDll, "UpdateHMDEmulationStatus"));
  g_dispatchTableForwarded.DXGID3D10CreateDevice = reinterpret_cast<DXGID3D10CreateDevicePtr>(
      GetProcAddress(g_systemDll, "DXGID3D10CreateDevice"));
  g_dispatchTableForwarded.DXGID3D10CreateLayeredDevice =
      reinterpret_cast<DXGID3D10CreateLayeredDevicePtr>(
          GetProcAddress(g_systemDll, "DXGID3D10CreateLayeredDevice"));
  g_dispatchTableForwarded.DXGID3D10GetLayeredDeviceSize =
      reinterpret_cast<DXGID3D10GetLayeredDeviceSizePtr>(
          GetProcAddress(g_systemDll, "DXGID3D10GetLayeredDeviceSize"));
  g_dispatchTableForwarded.DXGID3D10RegisterLayers = reinterpret_cast<DXGID3D10RegisterLayersPtr>(
      GetProcAddress(g_systemDll, "DXGID3D10RegisterLayers"));
  g_dispatchTableForwarded.DXGIReportAdapterConfiguration =
      reinterpret_cast<DXGIReportAdapterConfigurationPtr>(
          GetProcAddress(g_systemDll, "DXGIReportAdapterConfiguration"));

  auto* recorderWrapper =
      reinterpret_cast<gits::DirectX::IRecorderWrapper*>(g_gitsLoader->GetRecorderWrapperPtr());
  recorderWrapper->ExchangeDXGIDispatchTables(g_dispatchTableSystem, g_dispatchTableWrapper);
}

HRESULT WINAPI CreateDXGIFactory(REFIID riid, void** ppFactory) {
  initialize();
  return g_dispatchTableWrapper.CreateDXGIFactory(riid, ppFactory);
}

HRESULT WINAPI CreateDXGIFactory1(REFIID riid, void** ppFactory) {
  initialize();
  return g_dispatchTableWrapper.CreateDXGIFactory1(riid, ppFactory);
}

HRESULT WINAPI CreateDXGIFactory2(UINT Flags, REFIID riid, void** ppFactory) {
  initialize();
  return g_dispatchTableWrapper.CreateDXGIFactory2(Flags, riid, ppFactory);
}

HRESULT WINAPI DXGIDeclareAdapterRemovalSupport() {
  initialize();
  return g_dispatchTableWrapper.DXGIDeclareAdapterRemovalSupport();
}

HRESULT WINAPI DXGIDisableVBlankVirtualization() {
  initialize();
  return g_dispatchTableForwarded.DXGIDisableVBlankVirtualization();
}

HRESULT WINAPI DXGIGetDebugInterface(REFIID riid, void** pDebug) {
  initialize();
  return g_dispatchTableForwarded.DXGIGetDebugInterface(riid, pDebug);
}

HRESULT WINAPI ApplyCompatResolutionQuirking() {
  initialize();
  return g_dispatchTableForwarded.ApplyCompatResolutionQuirking();
}

HRESULT WINAPI CompatString() {
  initialize();
  return g_dispatchTableForwarded.CompatString();
}

HRESULT WINAPI CompatValue() {
  initialize();
  return g_dispatchTableForwarded.CompatValue();
}

HRESULT WINAPI DXGIDumpJournal() {
  initialize();
  return g_dispatchTableForwarded.DXGIDumpJournal();
}

HRESULT WINAPI PIXBeginCapture() {
  initialize();
  return g_dispatchTableForwarded.PIXBeginCapture();
}

HRESULT WINAPI PIXEndCapture() {
  initialize();
  return g_dispatchTableForwarded.PIXEndCapture();
}

HRESULT WINAPI PIXGetCaptureState() {
  initialize();
  return g_dispatchTableForwarded.PIXGetCaptureState();
}

HRESULT WINAPI SetAppCompatStringPointer() {
  initialize();
  return g_dispatchTableForwarded.SetAppCompatStringPointer();
}

HRESULT WINAPI UpdateHMDEmulationStatus() {
  initialize();
  return g_dispatchTableForwarded.UpdateHMDEmulationStatus();
}

HRESULT WINAPI DXGID3D10CreateDevice() {
  initialize();
  return g_dispatchTableForwarded.DXGID3D10CreateDevice();
}

HRESULT WINAPI DXGID3D10CreateLayeredDevice() {
  initialize();
  return g_dispatchTableForwarded.DXGID3D10CreateLayeredDevice();
}

HRESULT WINAPI DXGID3D10GetLayeredDeviceSize() {
  initialize();
  return g_dispatchTableForwarded.DXGID3D10GetLayeredDeviceSize();
}

HRESULT WINAPI DXGID3D10RegisterLayers() {
  initialize();
  return g_dispatchTableForwarded.DXGID3D10RegisterLayers();
}

HRESULT WINAPI DXGIReportAdapterConfiguration() {
  initialize();
  return g_dispatchTableForwarded.DXGIReportAdapterConfiguration();
}

HRESULT WINAPI DXGIGetDebugInterface1(UINT Flags, REFIID riid, void** pDebug) {
  initialize();
  return g_dispatchTableWrapper.DXGIGetDebugInterface1(Flags, riid, pDebug);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {

  switch (fdwReason) {
  case DLL_PROCESS_ATTACH:
    hookDxgi();
    break;
  case DLL_PROCESS_DETACH:
    if (!lpvReserved) {
      if (g_systemDll) {
        FreeLibrary(g_systemDll);
      }
      g_gitsLoader.reset();
    }
    break;
  }
  return TRUE;
}
