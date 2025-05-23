// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <sstream>
#include "tools.h"
#include "l0Header.h"

namespace gits {
  namespace l0 {
    template <typename T>
    struct IsPtrOrPtrPtr {
        static constexpr bool value = std::is_pointer_v<T> || std::is_pointer_v<std::remove_pointer_t<T>>;
    };
    std::string ToStringHelperExtensionStructs(const void* pNext);
    template <typename T,
              std::enable_if_t<std::is_arithmetic<T>::value, bool> = true>
    inline std::string ToStringHelperArithmetic(const T& handle) {
      return std::to_string(handle);
    }
    template <typename T,
              std::enable_if_t<!std::is_arithmetic<T>::value, bool> = true>
    inline std::string ToStringHelperArithmetic(const T& handle) {
      if (!handle) {
        return "0";
      }
      return gits::hex(handle).ToString();
    }

   template <typename T, typename = std::enable_if_t<!IsPtrOrPtrPtr<T>::value>>
   inline std::string ToStringHelper(const T& handle) { return ToStringHelperArithmetic(handle); }
   template <typename T, typename = std::enable_if_t<IsPtrOrPtrPtr<T>::value>>
   inline std::string ToStringHelper(T handle) { return ToStringHelperArithmetic(handle); }

    template<>
    inline std::string ToStringHelper(void** val) {
      std::stringstream ss;
      ss << ToStringHelper(reinterpret_cast<void*>(val));
      if (val != nullptr && gits::ShouldLog(LogLevel::TRACEV)) {
        ss << " { " << ToStringHelper(*val) << " }";
      }
      return ss.str();
    }
    template<>
    inline std::string ToStringHelper(uint64_t* val) {
      return ToStringHelper(reinterpret_cast<const void*>(val));
    }
    template<>
    inline std::string ToStringHelper(const void** val) {
      std::stringstream ss;
      ss << ToStringHelper(reinterpret_cast<const void*>(val));
      if (val != nullptr && gits::ShouldLog(LogLevel::TRACEV)) {
        ss << " { " << ToStringHelper(*val) << " }";
      }
      return ss.str();
    }
    template<>
    inline std::string ToStringHelper(uint32_t* val) {
      std::stringstream ss;
      ss << ToStringHelper(reinterpret_cast<void*>(val));
      if (val != nullptr && gits::ShouldLog(LogLevel::TRACEV)) {
        ss << " { " << ToStringHelper(*val) << " }";
      }
      return ss.str();
    }
%for name, enum in enums.items():
    template<>
    inline std::string ToStringHelper(const ${name}& val) {
  %if 'bitFields' in enum:
      std::stringstream ss;
      auto enumVal = static_cast<unsigned>(val);
    %for var in enum['vars']:
      if (${"enumVal == static_cast<unsigned>(" + var['name']+ ")" if var['value'] == '0' else "enumVal & static_cast<unsigned>(" + var['name'] + ")"}) {
        ss << ${'' if loop.first else '(ss.str().empty() ? "" : " | ") << '}"${var['name']}";
      }
    %endfor
      return ss.str().empty() ? std::to_string(enumVal) : ss.str();
  %else:
      switch (val) {
    %for var in enum['vars']:
        case ${var['name']}: return "${var['name']}";
    %endfor
        default: return std::to_string(static_cast<unsigned>(val));
      }
  %endif
    }
%endfor
    template<typename T, typename = std::enable_if_t<(sizeof(T) == 1U || (std::is_array_v<T> && sizeof(std::remove_all_extents_t<T>) == 1))>>
    inline std::string ToStringHelperArray(const T& handle, int size) {
      std::stringstream ss;
      ss << ToStringHelper(reinterpret_cast<const void*>(handle));
      constexpr auto maxSize = 8;
      size = maxSize < size ? maxSize : size;
      ss << " : { ";
      for (auto i = 0; i < size;) {
        ss << ToStringHelper(handle[i]);
        ss << ((++i < size) ? ", " : i >= maxSize ? ", ... }" : " }");
      }
      return ss.str();
    }
    template<typename T, typename = std::enable_if_t<(sizeof(T) > 1U)>>
    inline std::string ToStringHelperArray(const T& handle, [[maybe_unused]] size_t size) {
      std::stringstream ss;
      ss << ToStringHelper(reinterpret_cast<const void*>(handle));
      ss << " : { ... }";
      return ss.str();
    }
    inline std::string ToStringHelperHexMemoryView(const uint8_t* arg, const size_t &size) {
      std::stringstream result;
      if (arg == nullptr) {
        result << "nullptr";
        return result.str();
      }
      std::string dataStr = bytesToHex(arg, size);
      ReverseByPairs(dataStr);
      dataStr.erase(0, std::min(dataStr.find_first_not_of('0'), dataStr.size() - 1));
      dataStr.insert(0, "0x");
      result << dataStr;
      return result.str();
    }
    template <class T>
    inline std::string ToStringHelperArrayRange(const T& arg, const size_t &begin, const size_t &end) {
      std::stringstream result;
      if (arg) {
        result << arg;
      } else {
        result << "nullptr";
      }
      result << ": {";
      for (auto i = begin; i < end && arg != nullptr; i++) {
        result << "\n\t[" << i << "]: " << ToStringHelper(arg + i);
      }
      result << " }";
      return result.str();
    }
<%def name="struct_body(name, arg, ptr)">
      std::stringstream ss;
      ss << ToStringHelper(reinterpret_cast<const void*>(${"val" if ptr else "&val"}));
      if (gits::ShouldLog(LogLevel::TRACE)${" && val" if ptr else ""}) {
        ss << ": { ";
    %for var in arg['vars']:
        %if var['tag'] != '':
        if (gits::ShouldLog(LogLevel::TRACEV))
          ss << "${"[" + var['tag'] + "] "}";
        %endif
        %if '[' in var['name']:
        ss << ToStringHelperArray(${"val" + ("->" if ptr else ".") + get_field_name(var) + ", " + get_field_array_size(var)})${'' if loop.last else ' << ", "'};
        %elif 'pNext' in var['name']:
        ss << ToStringHelperExtensionStructs(${"val" + ("->" if ptr else ".") + var['name']})${'' if loop.last else ' << ", "'};
        %else:
        ss << ToStringHelper(${"val" + ("->" if ptr else ".") + var['name']})${'' if loop.last else ' << ", "'};
        %endif
    %endfor
        ss << " }";
      }
      return ss.str();\
</%def>
%for name, arg in arguments.items():
  %if 'vars' in arg:
    %if name in used_types:
    template<>
    inline std::string ToStringHelper(const ${name}& val) {\
      ${struct_body(name, arg, 0)}
    }
    %endif
    %if name + "*" in used_types:
    template<>
    inline std::string ToStringHelper(${name}* val) {\
      ${struct_body(name, arg, 1)}
    }
    %endif
    %if "const " + name + "*" in used_types:
    template<>
    inline std::string ToStringHelper(const ${name}* val) {\
      ${struct_body(name, arg, 1)}
    }
    %endif
  %elif "handle" in name and name + "*" in used_types:
    template<>
    inline std::string ToStringHelper(${name}* val) {
      std::stringstream ss;
      ss << ToStringHelper(reinterpret_cast<void*>(val));
      if (val != nullptr && gits::ShouldLog(LogLevel::TRACEV)) {
        ss << " { " << ToStringHelper(*val) << " }";
      }
      return ss.str();
    }
  %endif
%endfor
    inline std::string ToStringHelperExtensionStructs(const void* pNext) {
      if (pNext == nullptr) {
        return ToStringHelper(pNext);
      }
      const auto *extendedProperties =
          reinterpret_cast<const ze_base_properties_t *>(pNext);
%for name, enum in enums.items():
  %if name == "ze_structure_type_t":
      switch (extendedProperties->stype) {
      %for structure in enum['vars']:
      case ${structure['name']}:
        return ToStringHelper(reinterpret_cast<const ${structure['struct']}*>(pNext));
      %endfor
      default:
        return ToStringHelper(pNext);
      }
    %endif
 %endfor
    }
    class CL0Log : public gits::CLog {
    public:
      using gits::CLog::CLog;
      template <typename T,
                typename std::enable_if_t<std::is_arithmetic<T>::value, bool> = true>
      CL0Log &operator<<(const T &t) {
        _buffer << t;
        return *this;
      }
      template <typename T,
                typename std::enable_if_t<!std::is_arithmetic<T>::value, bool> = true>
      CL0Log &operator<<(const T &t) {
        _buffer << ToStringHelper<T>(t);
        return *this;
      }
      CL0Log &operator<<(const char c);
      CL0Log &operator<<(const unsigned char c);
      CL0Log &operator<<(const char *c);
      CL0Log &operator<<(std::string s);
      CL0Log &operator<<(char *c);
      CL0Log &operator<<(gits::manip t);
    };
  // See common/include/log.h for explanations of these macros.
#define L0Log1(lvl)                                                            ${'\\'}
  if (gits::ShouldLog(LogLevel::lvl))                                    ${'\\'}
  gits::l0::CL0Log(LogLevel::lvl, gits::LogStyle::NORMAL)
#define L0Log2(lvl, style)                                                     ${'\\'}
  if (gits::ShouldLog(LogLevel::lvl))                                    ${'\\'}
  gits::l0::CL0Log(LogLevel::lvl, gits::LogStyle::style)
  // Workaround for a MSVC bug, see https://stackoverflow.com/a/5134656/
#define EXPAND(x) x
  // Magic to call different variants based on the number of arguments.
#define GET_OVERLOAD(PLACEHOLDER1, PLACEHOLDER2, NAME, ...) NAME
#define L0Log(...)                                                             ${'\\'}
  EXPAND(GET_OVERLOAD(__VA_ARGS__, L0Log2, L0Log1)(__VA_ARGS__))
    void LogAppendKernel(const uint32_t& kernelNumber, const char* pKernelName);
    void LogKernelExecution(const uint32_t& kernelNumber,
                            const char* kernelName,
                            const uint32_t& cmdQueueNumber,
                            const uint32_t& cmdListNumber);
  } // namespace l0
} // namespace gits