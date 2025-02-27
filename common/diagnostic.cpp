// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "diagnostic.h"
#include "platform.h"
#include "gits.h"
#include "tools.h"
#include "pragmas.h"
#include "config.h"

#include <string>
#include <sstream>
#include <filesystem>

#if defined GITS_ARCH_X86
#define GITS_ARCH_STR "x86"
#elif defined GITS_ARCH_X64
#define GITS_ARCH_STR "x64"
#elif defined GITS_ARCH_ARM
#define GITS_ARCH_STR "ARM"
#elif defined GITS_ARCH_A64
#define GITS_ARCH_STR "AARCH64"
#endif

#ifdef GITS_PLATFORM_WINDOWS
#pragma comment(lib, "bcrypt.lib")
#endif

namespace gits {
namespace {
void gather_generic_diags(nlohmann::ordered_json& properties) {
  std::stringstream ver;
  ver << CGits::Instance().Version();
  properties["diag"]["gits"]["version"] = ver.str();
  properties["diag"]["gits"]["arch"] = GITS_ARCH_STR;
}
} // namespace
#ifdef GITS_PLATFORM_WINDOWS

//need to close namespace to pull additional headers
//and provide operator<< for VARIANT type
}

#pragma comment(lib, "wbemuuid.lib")

#define _WIN32_DCOM
#include <comdef.h>
#include <Wbemidl.h>
#include <windows.h>

namespace {
template <class T>
T get_element(void* ptr, int idx) {
  return static_cast<T*>(ptr)[idx];
}
} // namespace

std::ostream& operator<<(std::ostream& o, const VARIANT& var) {
  if (var.vt & VT_BYREF) {
    return o << "<ref>";
  }

  if (var.vt & VT_ARRAY) {
    SAFEARRAY* psa = V_ARRAY(&var);

    long lower, upper;
    SafeArrayGetLBound(psa, 1, &lower);
    SafeArrayGetUBound(psa, 1, &upper);

    if (lower == upper) {
      o << "<empty array>";
    }

    void* rawArray;
    SafeArrayAccessData(psa, &rawArray);

    const long count = upper - lower;
    for (long i = 0; i < count; ++i) {
      switch (var.vt & ~VT_ARRAY) {
      case VT_BOOL:
        o << (bool)get_element<VARIANT_BOOL>(rawArray, i);
        break;
      case VT_BSTR:
        o << _bstr_t(get_element<BSTR>(rawArray, i));
        break;
      case VT_CY:
        o << get_element<LONGLONG>(rawArray, i);
        break;
      case VT_DATE:
        o << get_element<DATE>(rawArray, i);
        break;
      case VT_DECIMAL:
        o << "<decimal>";
        break;
      case VT_DISPATCH:
        o << "<dispatch>";
        break;
      case VT_EMPTY:
        o << "<empty>";
        break;
      case VT_ERROR:
        o << get_element<SCODE>(rawArray, i);
        break;
      case VT_I1:
        o << (int)get_element<CHAR>(rawArray, i);
        break;
      case VT_I2:
        o << get_element<SHORT>(rawArray, i);
        break;
      case VT_I4:
        o << get_element<LONG>(rawArray, i);
        break;
      case VT_INT:
        o << get_element<INT>(rawArray, i);
        break;
      case VT_NULL:
        o << "<null>";
        break;
      case VT_R4:
        o << get_element<FLOAT>(rawArray, i);
        break;
      case VT_R8:
        o << get_element<DOUBLE>(rawArray, i);
        break;
      case VT_UI1:
        o << get_element<UCHAR>(rawArray, i);
        break;
      case VT_UI2:
        o << get_element<USHORT>(rawArray, i);
        break;
      case VT_UI4:
        o << get_element<ULONG>(rawArray, i);
        break;
      case VT_UINT:
        o << get_element<UINT>(rawArray, i);
        break;
      case VT_UNKNOWN:
        o << "<IUnknown>";
        break;
      case VT_VARIANT:
        o << "<variant>";
        break;
      default:
        o << "<unknown>";
        break;
      }
      o << ", ";
    }
    SafeArrayUnaccessData(psa);
    return o;
  }

  switch (var.vt) {
  case VT_BOOL:
    return o << (bool)var.boolVal;
  case VT_BSTR:
    return o << _bstr_t(var.bstrVal);
  case VT_CY:
    return o << var.cyVal.int64;
  case VT_DATE:
    return o << var.date;
  case VT_DECIMAL:
    return o << "<decimal>";
  case VT_DISPATCH:
    return o << "<dispatch>";
  case VT_EMPTY:
    return o << "<empty>";
  case VT_ERROR:
    return o << var.scode;
  case VT_I1:
    return o << (int)var.cVal;
  case VT_I2:
    return o << var.iVal;
  case VT_I4:
    return o << var.lVal;
  case VT_INT:
    return o << var.intVal;
  case VT_NULL:
    return o << "<null>";
  case VT_R4:
    return o << var.fltVal;
  case VT_R8:
    return o << var.dblVal;
  case VT_UI1:
    return o << (int)var.bVal;
  case VT_UI2:
    return o << var.uiVal;
  case VT_UI4:
    return o << var.ulVal;
  case VT_UINT:
    return o << var.uintVal;
  case VT_UNKNOWN:
    return o << "<IUnknown>";
  case VT_VARIANT:
    return o << "<variant>";
  default:
    return o << "<unknown>";
  }
}

//everything else will be in gits namespace
namespace gits {
namespace {
class WMIConnection : private gits::noncopyable {
public:
  WMIConnection() {
    HRESULT hres;
    hres = CoInitialize(0);
    if (FAILED(hres)) {
      throw std::runtime_error("Failed to initialize COM library.");
    }

    hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
                                RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);

    if (FAILED(hres)) {
      CoUninitialize();
      throw std::runtime_error("Failed to initialize security");
    }

    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator,
                            (LPVOID*)&pLoc);

    if (FAILED(hres)) {
      CoUninitialize();
      throw std::runtime_error("Failed to create IWbemLocator object.");
    }

    hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);

    if (FAILED(hres)) {
      pLoc->Release();
      CoUninitialize();
      throw std::runtime_error("Could not connect to WMI namespace.");
    }

    hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                             RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);

    if (FAILED(hres)) {
      pSvc->Release();
      pLoc->Release();
      CoUninitialize();
      throw std::runtime_error("Could not set proxy blanket.");
    }
  }

  ~WMIConnection() {
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();
  }

private:
  IWbemServices* pSvc;
  IWbemLocator* pLoc;
  friend class WMIClass;
};

class WMIClass : private gits::noncopyable {
public:
  WMIClass(WMIConnection& connection, const std::string& name)
      : con_(connection), pEnumerator(nullptr), pclsObj(nullptr) {
    std::string query = "SELECT * FROM " + name;
    HRESULT hres = con_.pSvc->ExecQuery(bstr_t("WQL"), bstr_t(query.c_str()),
                                        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL,
                                        &pEnumerator);

    if (FAILED(hres)) {
      throw std::runtime_error("WMI Query failed.");
    }
  }

  std::string get(const std::string& name) {
    VARIANT vtProp;

    std::wstring wname(name.size(), 0);
    std::copy(name.begin(), name.end(), wname.begin());
    HRESULT hres = pclsObj->Get(wname.c_str(), 0, &vtProp, 0, 0);
    if (FAILED(hres)) {
      return "<couldn't obtain property>";
    } else {
      std::stringstream retValstream;
      retValstream << vtProp;
      std::string retval = retValstream.str();
      VariantClear(&vtProp);
      return retval;
    }
  }

  bool next_result() {
    if (pEnumerator == 0) {
      return false;
    }

    ULONG uReturn = 0;
    if (pclsObj) {
      pclsObj->Release();
    }
    pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
    if (uReturn == 0) {
      pclsObj = 0;
      return false;
    }

    return true;
  }

  ~WMIClass() {
    pEnumerator->Release();
    if (pclsObj) {
      pclsObj->Release();
    }
  }

  WMIConnection& con_;
  IEnumWbemClassObject* pEnumerator;
  IWbemClassObject* pclsObj;
};

void DescribeClassImpl(WMIConnection& con,
                       const char* name,
                       const char** props,
                       int size,
                       nlohmann::ordered_json& properties) {
  WMIClass cl(con, name);
  while (cl.next_result()) {
    properties["diag"]["os_specific"];
    for (int i = 0; i < size; ++i) {
      properties["diag"]["os_specific"][name][props[i]] = cl.get(props[i]);
    }
  }
}

template <int S>
void DescribeClass(WMIConnection& con,
                   const char* name,
                   const char* (&props)[S],
                   nlohmann::ordered_json& properties) {
  DescribeClassImpl(con, name, props, S, properties);
}

} // namespace

void gather_diagnostic_info(nlohmann::ordered_json& properties) {

  {
    char exePathArr[MAX_PATH];
    GetModuleFileName(NULL, exePathArr, sizeof(exePathArr));
    std::filesystem::path exePath(exePathArr);
    if (std::filesystem::exists(exePath)) {
      std::string exeName = exePath.filename().string();
      properties["diag"]["app"]["name"] = exeName;
      properties["diag"]["app"]["path"] = exePath;
      properties["diag"]["original_app"]["name"] = exeName;
    }
  }

  properties["diag"]["proc"]["cmdline"] = GetCommandLine();

  {
    std::time_t now = std::time(0);
    auto localTime = std::localtime(&now);
    std::stringstream currentDate;
    currentDate << std::put_time(localTime, "%d.%m.%Y");
    properties["diag"]["datetime"]["date"] = currentDate.str();
    std::stringstream currentTime;
    currentTime << std::put_time(localTime, "%H:%M:%S");
    properties["diag"]["datetime"]["time"] = currentTime.str();
  }

  {
    std::stringstream res;
    res << GetSystemMetrics(SM_CXSCREEN) << "x" << GetSystemMetrics(SM_CYSCREEN);
    properties["diag"]["screen"]["resolution"] = res.str();
  }

  try {
    if (Config::Get().common.recorder.extendedDiagnosticInfo) {
      Log(INFO) << "Gathering extended diagnostic info trough WMI...";
      Log(INFO) << "If it hangs or fails, consider disabling extended "
                   "diagnostics in recorder settings.";
      WMIConnection con;
      {
        const char* props[] = {
            "AcceleratorCapabilities",
            "AdapterCompatibility",
            "AdapterDACType",
            "AdapterRAM",
            "Availability",
            "CapabilityDescriptions",
            "Caption",
            "ColorTableEntries",
            "ConfigManagerErrorCode",
            "ConfigManagerUserConfig",
            "CreationClassName",
            "CurrentBitsPerPixel",
            "CurrentHorizontalResolution",
            "CurrentNumberOfColors",
            "CurrentNumberOfColumns",
            "CurrentNumberOfRows",
            "CurrentRefreshRate",
            "CurrentScanMode",
            "CurrentVerticalResolution",
            "Description",
            "DeviceID",
            "DeviceSpecificPens",
            "DitherType",
            "DriverDate",
            "DriverVersion",
            "ErrorCleared",
            "ErrorDescription",
            "ICMIntent",
            "ICMMethod",
            "InfFilename",
            "InfSection",
            "InstallDate",
            "InstalledDisplayDrivers",
            "LastErrorCode",
            "MaxMemorySupported",
            "MaxNumberControlled",
            "MaxRefreshRate",
            "MinRefreshRate",
            "Monochrome",
            "Name",
            "NumberOfColorPlanes",
            "NumberOfVideoPages",
            "PNPDeviceID",
            "PowerManagementCapabilities",
            "PowerManagementSupported",
            "ProtocolSupported",
            "ReservedSystemPaletteEntries",
            "SpecificationVersion",
            "Status",
            "StatusInfo",
            "SystemCreationClassName",
            "SystemPaletteEntries",
            "TimeOfLastReset",
            "VideoArchitecture",
            "VideoMemoryType",
            "VideoMode",
            "VideoModeDescription",
            "VideoProcessor",
        };
        DescribeClass(con, "Win32_VideoController", props, properties);
      }
      {
        const char* props[] = {
            "Availability",
            "Bandwidth",
            "Caption",
            "ConfigManagerErrorCode",
            "ConfigManagerUserConfig",
            "CreationClassName",
            "Description",
            "DeviceID",
            "DisplayType",
            "ErrorCleared",
            "ErrorDescription",
            "InstallDate",
            "IsLocked",
            "LastErrorCode",
            "MonitorManufacturer",
            "MonitorType",
            "Name",
            "PixelsPerXLogicalInch",
            "PixelsPerYLogicalInch",
            "PNPDeviceID",
            "PowerManagementCapabilities",
            "PowerManagementSupported",
            "ScreenHeight",
            "ScreenWidth",
            "Status",
            "StatusInfo",
            "SystemCreationClassName",
        };
        DescribeClass(con, "Win32_DesktopMonitor", props, properties);
      }
      {
        const char* props[] = {
            "BiosCharacteristics",
            "BIOSVersion",
            "BuildNumber",
            "Caption",
            "CodeSet",
            "CurrentLanguage",
            "Description",
            "IdentificationCode",
            "InstallableLanguages",
            "InstallDate",
            "LanguageEdition",
            "ListOfLanguages",
            "Manufacturer",
            "Name",
            "OtherTargetOS",
            "PrimaryBIOS",
            "ReleaseDate",
            "SMBIOSBIOSVersion",
            "SMBIOSMajorVersion",
            "SMBIOSMinorVersion",
            "SMBIOSPresent",
            "SoftwareElementID",
            "SoftwareElementState",
            "Status",
            "TargetOperatingSystem",
            "Version",
        };
        DescribeClass(con, "Win32_BIOS", props, properties);
      }
      {
        const char* props[] = {
            "BootDirectory", "Caption",          "ConfigurationPath", "Description",   "LastDrive",
            "Name",          "ScratchDirectory", "SettingID",         "TempDirectory",
        };
        DescribeClass(con, "Win32_BootConfiguration", props, properties);
      }
      {
        const char* props[] = {
            "Access",
            "AdditionalErrorData",
            "Associativity",
            "Availability",
            "BlockSize",
            "CacheSpeed",
            "CacheType",
            "Caption",
            "ConfigManagerErrorCode",
            "ConfigManagerUserConfig",
            "CorrectableError",
            "CreationClassName",
            "CurrentSRAM",
            "Description",
            "DeviceID",
            "EndingAddress",
            "ErrorAccess",
            "ErrorAddress",
            "ErrorCleared",
            "ErrorCorrectType",
            "ErrorData",
            "ErrorDataOrder",
            "ErrorDescription",
            "ErrorInfo",
            "ErrorMethodology",
            "ErrorResolution",
            "ErrorTime",
            "ErrorTransferSize",
            "FlushTimer",
            "InstallDate",
            "InstalledSize",
            "LastErrorCode",
            "Level",
            "LineSize",
            "Location",
            "MaxCacheSize",
            "Name",
            "NumberOfBlocks",
            "OtherErrorDescription",
            "PNPDeviceID",
            "PowerManagementCapabilities",
            "PowerManagementSupported",
            "Purpose",
            "ReadPolicy",
            "ReplacementPolicy",
            "StartingAddress",
            "Status",
            "StatusInfo",
            "SupportedSRAM",
            "SystemCreationClassName",
            "SystemLevelAddress",
            "WritePolicy",
        };
        DescribeClass(con, "Win32_CacheMemory", props, properties);
      }
      {
        const char* props[] = {
            "BankLabel",
            "Capacity",
            "Caption",
            "CreationClassName",
            "DataWidth",
            "Description",
            "DeviceLocator",
            "FormFactor",
            "HotSwappable",
            "InstallDate",
            "InterleaveDataDepth",
            "InterleavePosition",
            "Manufacturer",
            "MemoryType",
            "Model",
            "Name",
            "OtherIdentifyingInfo",
            "PositionInRow",
            "PoweredOn",
            "Removable",
            "Replaceable",
            "SKU",
            "Speed",
            "Status",
            "Tag",
            "TotalWidth",
            "TypeDetail",
            "Version",
        };
        DescribeClass(con, "Win32_PhysicalMemory", props, properties);
      }
      {
        const char* props[] = {
            "AddressWidth",
            "Architecture",
            "Availability",
            "Caption",
            "ConfigManagerErrorCode",
            "ConfigManagerUserConfig",
            "CpuStatus",
            "CreationClassName",
            "CurrentClockSpeed",
            "CurrentVoltage",
            "DataWidth",
            "Description",
            "DeviceID",
            "ErrorCleared",
            "ErrorDescription",
            "ExtClock",
            "Family",
            "InstallDate",
            "L2CacheSize",
            "L2CacheSpeed",
            "L3CacheSize",
            "L3CacheSpeed",
            "LastErrorCode",
            "Level",
            "LoadPercentage",
            "Manufacturer",
            "MaxClockSpeed",
            "Name",
            "NumberOfCores",
            "NumberOfLogicalProcessors",
            "OtherFamilyDescription",
            "PNPDeviceID",
            "PowerManagementCapabilities",
            "PowerManagementSupported",
            "ProcessorType",
            "Revision",
            "Role",
            "SocketDesignation",
            "Status",
            "StatusInfo",
            "Stepping",
            "SystemCreationClassName",
            "UpgradeMethod",
            "Version",
            "VoltageCaps",
        };
        DescribeClass(con, "Win32_Processor", props, properties);
      }
      {
        const char* props[] = {
            "AdminPasswordStatus",
            "AutomaticManagedPagefile",
            "AutomaticResetBootOption",
            "AutomaticResetCapability",
            "BootOptionOnLimit",
            "BootOptionOnWatchDog",
            "BootROMSupported",
            "BootupState",
            "Caption",
            "ChassisBootupState",
            "CreationClassName",
            "CurrentTimeZone",
            "DaylightInEffect",
            "Description",
            "DNSHostName",
            "Domain",
            "DomainRole",
            "EnableDaylightSavingsTime",
            "FrontPanelResetStatus",
            "InfraredSupported",
            "InitialLoadInfo",
            "InstallDate",
            "KeyboardPasswordStatus",
            "LastLoadInfo",
            "Manufacturer",
            "Model",
            "Name",
            "NameFormat",
            "NetworkServerModeEnabled",
            "NumberOfLogicalProcessors",
            "NumberOfProcessors",
            "OEMStringArray",
            "PartOfDomain",
            "PauseAfterReset",
            "PCSystemType",
            "PowerManagementCapabilities",
            "PowerManagementSupported",
            "PowerOnPasswordStatus",
            "PowerState",
            "PowerSupplyState",
            "PrimaryOwnerContact",
            "PrimaryOwnerName",
            "ResetCapability",
            "ResetCount",
            "ResetLimit",
            "Roles",
            "Status",
            "SupportContactDescription",
            "SystemStartupDelay",
            "SystemStartupOptions",
            "SystemStartupSetting",
            "SystemType",
            "ThermalState",
            "TotalPhysicalMemory",
            "WakeUpType",
            "Workgroup",
        };
        DescribeClass(con, "Win32_ComputerSystem", props, properties);
      }
      {
        const char* props[] = {
            "Caption", "Description", "Name", "SKUNumber", "Vendor", "Version",

        };
        DescribeClass(con, "Win32_ComputerSystemProduct", props, properties);
      }
      {
        const char* props[] = {
            "Day",   "DayOfWeek", "Hour",   "Milliseconds", "Minute",
            "Month", "Quarter",   "Second", "WeekInMonth",  "Year",
        };
        DescribeClass(con, "Win32_CurrentTime", props, properties);
      }
      {
        const char* props[] = {
            "BootDevice",
            "BuildNumber",
            "BuildType",
            "Caption",
            "CodeSet",
            "CountryCode",
            "CreationClassName",
            "CSCreationClassName",
            "CSDVersion",
            "CSName",
            "CurrentTimeZone",
            "DataExecutionPrevention_Available",
            "DataExecutionPrevention_32BitApplications",
            "DataExecutionPrevention_Drivers",
            "DataExecutionPrevention_SupportPolicy",
            "Debug",
            "Description",
            "Distributed",
            "EncryptionLevel",
            "ForegroundApplicationBoost",
            "FreePhysicalMemory",
            "FreeSpaceInPagingFiles",
            "FreeVirtualMemory",
            "InstallDate",
            "LargeSystemCache",
            "LastBootUpTime",
            "LocalDateTime",
            "Locale",
            "Manufacturer",
            "MaxNumberOfProcesses",
            "MaxProcessMemorySize",
            "MUILanguages",
            "Name",
            "NumberOfLicensedUsers",
            "NumberOfProcesses",
            "NumberOfUsers",
            "OperatingSystemSKU",
            "Organization",
            "OSArchitecture",
            "OSLanguage",
            "OSProductSuite",
            "OSType",
            "OtherTypeDescription",
            "PAEEnabled",
            "PlusProductID",
            "PlusVersionNumber",
            "PortableOperatingSystem",
            "Primary",
            "ProductType",
            "RegisteredUser",
            "ServicePackMajorVersion",
            "ServicePackMinorVersion",
            "SizeStoredInPagingFiles",
            "Status",
            "SuiteMask",
            "SystemDevice",
            "SystemDirectory",
            "SystemDrive",
            "TotalSwapSpaceSize",
            "TotalVirtualMemorySize",
            "TotalVisibleMemorySize",
            "Version",
            "WindowsDirectory",
        };
        DescribeClass(con, "Win32_OperatingSystem", props, properties);
      }
      {
        const char* props[] = {
            "AccessMask",
            "Archive",
            "Caption",
            "Compressed",
            "CompressionMethod",
            "CreationClassName",
            "CreationDate",
            "CSCreationClassName",
            "CSName",
            "Description",
            "Drive",
            "EightDotThreeFileName",
            "Encrypted",
            "EncryptionMethod",
            "Extension",
            "FileName",
            "FileSize",
            "FileType",
            "FreeSpace",
            "FSCreationClassName",
            "FSName",
            "Hidden",
            "InitialSize",
            "InstallDate",
            "InUseCount",
            "LastAccessed",
            "LastModified",
            "Manufacturer",
            "MaximumSize",
            "Name",
            "Path",
            "Readable",
            "Status",
            "System",
            "Version",
            "Writeable",
        };
        DescribeClass(con, "Win32_PageFile", props, properties);
      }
      {
        const char* props[] = {
            "Bias",
            "Caption",
            "DaylightBias",
            "DaylightDay",
            "DaylightDayOfWeek",
            "DaylightHour",
            "DaylightMillisecond",
            "DaylightMinute",
            "DaylightMonth",
            "DaylightName",
            "DaylightSecond",
            "DaylightYear",
            "Description",
            "SettingID",
            "StandardBias",
            "StandardDay",
            "StandardDayOfWeek",
            "StandardHour",
            "StandardMillisecond",
            "StandardMinute",
            "StandardMonth",
            "StandardName",
            "StandardSecond",
            "StandardYear",
        };
        DescribeClass(con, "Win32_TimeZone", props, properties);
      }
    }
  } catch (std::exception& e) {
    properties["diag"]["os_specific"] = e.what();
  }

  gather_generic_diags(properties);

  properties["diag"]["gits"]["os"] = "Windows";
}

#elif defined GITS_PLATFORM_X11

#include <unistd.h>

void gather_diagnostic_info(nlohmann::ordered_json& properties) {
  auto my_pid = std::to_string(getpid());
  if (Config::Get().common.recorder.extendedDiagnosticInfo) {
    properties["diag"]["gits"]["uname"] =
        CommandOutput("uname -a", Config::Get().common.mode == Config::MODE_RECORDER);
    properties["diag"]["gits"]["lspci"] =
        CommandOutput("lspci -v", Config::Get().common.mode == Config::MODE_RECORDER);
    properties["diag"]["gits"]["maps"] = CommandOutput(
        "cat /proc/" + my_pid + "/maps", Config::Get().common.mode == Config::MODE_RECORDER);
    properties["diag"]["gits"]["cpuinfo"] =
        CommandOutput("cat /proc/cpuinfo", Config::Get().common.mode == Config::MODE_RECORDER);
  }

  gather_generic_diags(properties);
  properties["diag"]["gits"]["os"] = "Linux";

  std::string cmdline = CommandOutput("cat /proc/" + my_pid + "/cmdline | sed 's/\\x0/ /g'",
                                      Config::Get().common.mode == Config::MODE_RECORDER);
  properties["diag"]["gits"]["cmdline"] = cmdline;
}

#else

void gather_diagnostic_info(nlohmann::ordered_json& properties) {
  gather_generic_diags(properties);
  properties["diag"]["gits"]["os"] = "<unknown>";
}

#endif
} // namespace gits
