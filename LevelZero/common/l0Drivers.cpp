// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "l0Drivers.h"

#include "gits.h"
#include "l0ApisIface.h"

#include <string>

#ifdef GITS_PLATFORM_WINDOWS
#include <initguid.h>
#include <Windows.h>
#include <devpropdef.h>
#include <devpkey.h>
#include <Shlwapi.h>
#include <ntddvdeo.h>
#include <stdlib.h>
#include <SetupAPI.h>
namespace {
bool GetPropertyFromDevice(void* pDevInfo,
                           PSP_DEVINFO_DATA pDevInfoData,
                           const DEVPROPKEY* pPropertyKey,
                           wchar_t** ppStringOut,
                           unsigned long* pStringOutSize) {
  unsigned long propertyType = 0;
  unsigned long propertySize = 0;
  // request a size, in bytes, required for a buffer in which property value
  // will be stored SetupDiGetDeviceProperty() returns false and
  // ERROR_INSUFFICIENT_BUFFER for the call
  if (SetupDiGetDevicePropertyW(pDevInfo, pDevInfoData, pPropertyKey, &propertyType, NULL, 0,
                                &propertySize, 0)) {
    return false;
  }
  if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
    return false;
  }
  if (ppStringOut == NULL) {
    SetLastError(ERROR_INVALID_HANDLE);
    return false;
  }
  // allocate memory for the buffer
  if (propertySize > 0) {
    *ppStringOut = new wchar_t[propertySize];
  }
  if (*ppStringOut == NULL) {
    SetLastError(ERROR_INVALID_HANDLE);
    return false;
  }
  // fill in the buffer with property value
  if (!SetupDiGetDevicePropertyW(pDevInfo, pDevInfoData, pPropertyKey, &propertyType,
                                 reinterpret_cast<PBYTE>(*ppStringOut), propertySize, NULL, 0)) {
    delete[] *ppStringOut;
    *ppStringOut = NULL;
    return false;
  }
  if (pStringOutSize) {
    *pStringOutSize = propertySize;
  }
  return true;
}

bool GetIntelDriverStoreFullPath(wchar_t* pDriverStorePath,
                                 unsigned long driverStorePathSizeInCharacters,
                                 unsigned long* pDriverStorePathLengthInCharacters) {
  constexpr wchar_t vendorName_[] = L"intel";
  bool result = false;
  wchar_t* pPropertyInfName = nullptr;
  wchar_t* pPropertyDevServiceName = nullptr;
  // guid defined for display adapters
  const GUID guid = GUID_DISPLAY_DEVICE_ARRIVAL;

  // create device information set containing dispaly adapters which support
  // interfaces and are currently present in the system
  void* pDevInfo = SetupDiGetClassDevs(&guid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
  if (pDevInfo == INVALID_HANDLE_VALUE) {
    goto END;
  }
  unsigned long deviceIndex = 0;
  SP_DEVINFO_DATA devInfoData;
  ZeroMemory(&devInfoData, sizeof(SP_DEVINFO_DATA));
  unsigned long interfaceIndex = 0;
  SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
  ZeroMemory(&deviceInterfaceData, sizeof(SP_DEVICE_INTERFACE_DATA));
  DEVPROPKEY devPropKey;
  ZeroMemory(&devPropKey, sizeof(DEVPROPKEY));
  unsigned long propertyDevServiceNameLength = 0;
  unsigned long driverStorePathLengthInCharacters = 0;

  // enumerate dispaly adapters
  while (true) {
    ZeroMemory(&devInfoData, sizeof(SP_DEVINFO_DATA));
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    if (!SetupDiEnumDeviceInfo(pDevInfo, deviceIndex, &devInfoData)) {
      deviceIndex = 0;
      goto END;
    }
    // enumerate interfaces of display adapters
    while (true) {
      ZeroMemory(&deviceInterfaceData, sizeof(SP_DEVICE_INTERFACE_DATA));
      deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

      if (!SetupDiEnumDeviceInterfaces(pDevInfo, &devInfoData, &guid, interfaceIndex,
                                       &deviceInterfaceData)) {
        if (GetLastError() == ERROR_NO_MORE_ITEMS) {
          interfaceIndex = 0;
          break;
        } else {
          interfaceIndex = 0;
          goto END;
        }
      }
      // get an inf file name for a display adapter
      if (GetPropertyFromDevice(pDevInfo, &devInfoData, &DEVPKEY_Device_DriverInfPath,
                                &pPropertyInfName, NULL) == false) {
        goto END;
      }
      wchar_t driverStorePath[MAX_PATH];
      ZeroMemory(driverStorePath, sizeof(driverStorePath));
      // get a fully qualified name of an inf file (directory path and file
      // name)
      if (!SetupGetInfDriverStoreLocationW(reinterpret_cast<PCWSTR>(pPropertyInfName), NULL, NULL,
                                           reinterpret_cast<PWSTR>(driverStorePath),
                                           ARRAYSIZE(driverStorePath), NULL)) {
        goto END;
      }
      // remove backslash and file name from the fully qualified name
      PathRemoveFileSpecW(reinterpret_cast<LPWSTR>(driverStorePath));
      driverStorePathLengthInCharacters =
          static_cast<unsigned long>(wcsnlen_s(driverStorePath, ARRAYSIZE(driverStorePath)));
      // get service name for a display adapter
      if (GetPropertyFromDevice(pDevInfo, &devInfoData, &DEVPKEY_Device_Manufacturer,
                                &pPropertyDevServiceName, &propertyDevServiceNameLength) == false ||
          pPropertyDevServiceName == nullptr) {
        goto END;
      }
      // to read DEVPKEY_Device_Service property value correctly just cast
      // unsigned char* (means PBYTE) to const wchar_t*
      _wcslwr_s(pPropertyDevServiceName, propertyDevServiceNameLength);
      wchar_t* pDevServiceName = pPropertyDevServiceName;
      // check if a given display adapter is from intel based on driver
      // device service name "igfx"
      if ((wcsncmp(pDevServiceName, vendorName_, wcslen(vendorName_)) == 0)) {
        if (pDriverStorePath == NULL) {
          goto END;
        }
        if (driverStorePathSizeInCharacters < driverStorePathLengthInCharacters + 1) {
          goto END;
        }
        wcscpy_s(pDriverStorePath, driverStorePathSizeInCharacters, driverStorePath);

        if (pDriverStorePathLengthInCharacters) {
          *pDriverStorePathLengthInCharacters = driverStorePathLengthInCharacters;
        }
        result = (driverStorePathLengthInCharacters > 0) ? true : false;
        if (result == false) {
          SetLastError(ERROR_BAD_LENGTH);
        }
        goto END;
      } else {
        if (pPropertyDevServiceName != nullptr) {
          delete[] pPropertyDevServiceName;
          pPropertyDevServiceName = nullptr;
        }
        if (pPropertyInfName != nullptr) {
          delete[] pPropertyInfName;
          pPropertyInfName = nullptr;
        }
      }
      ++interfaceIndex;
    }
    ++deviceIndex;
  }

END:
  if (pPropertyDevServiceName != nullptr) {
    delete[] pPropertyDevServiceName;
    pPropertyDevServiceName = nullptr;
  }

  if (pPropertyInfName != nullptr) {
    delete[] pPropertyInfName;
    pPropertyInfName = nullptr;
  }

  if (pDevInfo != nullptr) {
    SetupDiDestroyDeviceInfoList(pDevInfo);
    pDevInfo = nullptr;
  }
  return result;
}
} // namespace
#endif

namespace gits {
namespace l0 {
CDriver drv;

// constructor in l0DriversInit.cpp

CDriver::~CDriver() {
  initialized_ = false;
}

bool CDriver::OpenLibrary(const std::string& path) {
  LOG_TRACE << "Using LibL0: " << path;
  lib_ = std::make_unique<SharedLibrary>(path.c_str());
  initialized_ = lib_->getHandle() != nullptr;
  return initialized_;
}

void CDriver::Initialize() {
  if (initialized_) {
    return;
  }
  std::string path = Configurator::Get().common.shared.libL0.string();
  gits::CGits::Instance().apis.UseApiComputeIface(std::make_shared<Api>());
  if (OpenLibrary(path)) {
    return;
  }
  path = Configurator::Get().common.shared.libL0Driver.string();
#ifdef GITS_PLATFORM_WINDOWS
  unsigned long driverStorePathLengthInCharacters = 0;
  wchar_t driverStorePath[MAX_PATH] = {0};
  if (GetIntelDriverStoreFullPath(driverStorePath, ARRAYSIZE(driverStorePath),
                                  &driverStorePathLengthInCharacters) == true) {
    std::wstring driverStoreFullPath(driverStorePath);
    if (driverStorePathLengthInCharacters) {
      driverStoreFullPath += L"\\";
      driverStoreFullPath += std::wstring(path.begin(), path.end()).c_str();
    }
    const wchar_t* libPath = driverStoreFullPath.c_str();
    const size_t size = std::wcslen(libPath) * sizeof(wchar_t);
    char* buffer = new char[(size + 1) * MB_CUR_MAX];
    std::wcstombs(buffer, libPath, size);
    path = std::string(buffer);
    delete[] buffer;
    OpenLibrary(path);
  }
#else
  OpenLibrary(path);
#endif
}

bool CDriver::IsInitialized() const {
  return initialized_;
}
} // namespace l0
} // namespace gits
