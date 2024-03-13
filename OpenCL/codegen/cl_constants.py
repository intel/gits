#!python
#!/usr/bin/env python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

COPYRIGHT_HEADER = """//====================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
//====================== end_copyright_notice ==============================

"""

CALLBACKS = {
    'CCallbackProgram': 'void (CL_CALLBACK *{name})(cl_program, void*)',
    'CCallbackContext': 'void (CL_CALLBACK *{name})(const char*, const void*, size_t, void*)',
    'CCallbackEvent': 'void (CL_CALLBACK *{name})(cl_event, cl_int, void*)',
    'CCallbackMem': 'void (CL_CALLBACK *{name})(cl_mem, void*)',
    'CCallbackFunc': 'void (CL_CALLBACK *{name})(void*)',
    'CCallbackSVM': 'void (CL_CALLBACK *{name})(cl_command_queue, cl_uint, void**, void*)',
}

WRAPPER_HEADER_NAME = 'openclRecorderWrapperAuto.h'
WRAPPER_INTERFACE_HEADER_NAME = 'openclRecorderWrapperIfaceAuto.h'
WRAPPER_SOURCE_NAME = 'openclRecorderWrapperAuto.cpp'

WRAPPER_SOURCE_START = """
#include "openclRecorderWrapper.h"
#include "openclRecorderSubwrappers.h"
#include "openclFunctionsAuto.h"
#include "openclStateTracking.h"
#include "openclTools.h"
#include "recorder.h"

namespace gits {
  namespace OpenCL {
"""

WRAPPER_SOURCE_END = """
} // namespace OpenCL
} // namespace gits
"""

WRAPPER_FUNCTION = """
void CRecorderWrapper::{name}(
  {arguments}) const
{{
  {rec_wrap}
}}
"""

WRAPPER_DEFAULT = """
  CFunction *_token = nullptr;
  if({condition})
  {{
    {body}
  }}
  {state_dynamic}
"""

WRAPPER_SCHEDULE = '_token = new C{call};\n    _recorder.Schedule(_token);'
WRAPPER_NOT_IMPLEMENTED = 'Log(ERR) << "{name} not implemented.";'

RECEXECWRAP_NAME = '../../OpenCL/interceptor/include/openclExecWrap.h'

RECEXECWRAP_START = """
#include "openclDrivers.h"
#include "openclRecorderWrapperIface.h"
#include "gitsPluginOpenCL.h"

#include "exception.h"
#include "gits.h"
#include "log.h"
#include "platform.h"

#include <map>

namespace {
  std::recursive_mutex globalMutex;
}

using gits::Log;
using namespace gits::OpenCL;

#define OCLATTRIB
namespace {
  // Avoid recording API - recursive functions.
  uint32_t recursionDepth = 0;
  const uint32_t disableDepth = 1000;
}

void PrePostDisableOpenCL() {
  recursionDepth = disableDepth;
}

#define GITS_WRAPPER_PRE                                           \
  --recursionDepth;                                                \
  if (CGitsPluginOpenCL::Configuration().recorder.basic.enabled && \
     (recursionDepth == 0)) {                                      \
    try {

#define GITS_WRAPPER_POST                      \
      wrapper.CloseRecorderIfRequired();       \
    }                                          \
    catch (...) {                              \
      topmost_exception_handler(__FUNCTION__); \
    }                                          \
  }

#define GITS_ENTRY                                                  \
  ++recursionDepth;                                                 \
  CGitsPluginOpenCL::Initialize();                                  \
  IRecorderWrapper& wrapper = CGitsPluginOpenCL::RecorderWrapper(); \
  COclDriver &drvOcl = wrapper.Drivers();                           \
  wrapper.InitializeDriver();

#define GITS_MUTEX std::unique_lock<std::recursive_mutex> lock(globalMutex);
#define GITS_ENTRY_OCL GITS_MUTEX GITS_ENTRY

namespace gits {
  namespace OpenCL {
    void* GetExtensionFunction(const char* function_name);
"""

RECEXECWRAP_END = """
} // namespace OpenCL
} // namespace gits
"""

RECEXECWRAP_FUNCTION = """
    inline {rettype} {name}_RECEXECWRAP({args})
    {{

    }}
"""

PREPOST_NAME = 'openclPrePostAuto.cpp'

PREPOST_START = """
#include "openclExecWrap.h"

extern "C" {
"""

PREPOST_END = """
}
"""

PREPOST_FUNCTION_BODY = """
  GITS_ENTRY_OCL
  {auto}drv.{drv_call};
  GITS_WRAPPER_PRE
    wrapper.{wrapper_call};
  GITS_WRAPPER_POST{retval}
"""

PREPOST_FUNCTION = """
CL_API_ENTRY VISIBLE {rettype} CL_API_CALL {name}(
  {arguments}) CL_API_SUFFIX__VERSION_{version}
{{{body}}}
"""

PREPOST_FUNCTION_EXT = """
CL_API_ENTRY VISIBLE {rettype} CL_API_CALL {name}(
  {arguments})
CL_API_SUFFIX__VERSION_{version}
{{{body}}}
"""

PREPOST_EXTENSION_FUNCTION = """
namespace gits {{
namespace OpenCL {{
void* GetExtensionFunction(const char* function_name) {{
  static const std::map<std::string, void*> funcMap{{
{0}
  }};
  auto iter = funcMap.find(function_name);
  if (iter == funcMap.end()) {{
    Log(WARN) << {1} << function_name << {2};
    return nullptr;
  }}
  return iter->second;
}}
}} // namespace OpenCL
}} // namespace gits
"""

PREPOST_EXTENSION_FUNCTION_ENTRY = '        {{ "{0}", reinterpret_cast<void*>({0}) }},\n'

DRIVERS_NAME = 'openclDriversAuto.h'

DRIVERS_START = '#define OCL_FUNCTIONS(a)'
DRIVERS_FUNCTION = '  OCL_FUNCTION(a, {rettype}, {name}, ({args_type}), ({args}), ({trace_args}))'
DRIVERS_ENUM_START = '#define OCL_TYPES'
DRIVERS_ENUM = '  OCL_TYPE({})'

SWITCH_NAME = 'openclIDswitch.h'

IDS_NAME = 'openclIDs.h'

TOKENS_HEADER_NAME = 'openclFunctionsAuto.h'
TOKENS_SOURCE_NAME = 'openclFunctionsAuto.cpp'

TOKENS_HEADER_START = """
#pragma once

#include "platform.h"
#include "openclHeader.h"
#include "gits.h"
#include "oclFunction.h"
#include "openclArguments.h"
#include "openclLibrary.h"
#include "pragmas.h"

namespace gits {
namespace OpenCL {
"""

TOKENS_HEADER_END = """
} // namespace OpenCL
} // namespace gits
"""

TOKENS_SOURCE_START = """
#include "openclFunctionsAuto.h"
#include "openclDrivers.h"
#include "openclPlayerRunWrap.h"
#include "openclStateTracking.h"
#include "exception.h"
#include "log.h"
"""

TOKENS_HEADER_TEMPLATE = """
    class C{name} : public CFunction {{
      static const unsigned ARG_NUM = {arg_num};
      static const unsigned RESULT_NUM = {result_num};

      {properties}

      virtual unsigned ArgumentCount() const {{ return ARG_NUM; }}
      virtual CArgument &Argument(unsigned idx);
      virtual const CArgument* Return() const {{ return &_return_value; }}
      virtual unsigned ResultCount() const {{ return RESULT_NUM; }}
      virtual CArgument &Result(unsigned idx);

    public:
      C{name}() {{}}{constructor}
      virtual unsigned int Id() const {{ return {id}; }}
      virtual const char *Name() const {{ return "{key_name}"; }}
      virtual unsigned Version() const {{ return VERSION_{version}; }}
      virtual void Run();{ccode}{write_post_call}
    }};

"""

TOKENS_HEADER_TEMPLATE_VOID = """
    class C{name} : public CFunction {{
      static const unsigned ARG_NUM = {arg_num};
      static const unsigned RESULT_NUM = {result_num};

      {properties}

      virtual unsigned ArgumentCount() const {{ return ARG_NUM; }}
      virtual CArgument &Argument(unsigned idx);
      virtual const CArgument* Return() const {{ return nullptr; }}
      virtual unsigned ResultCount() const {{ return RESULT_NUM; }}
      virtual CArgument &Result(unsigned idx);

    public:
      C{name}() {{}}{constructor}
      virtual unsigned int Id() const {{ return {id}; }}
      virtual const char *Name() const {{ return "{name}"; }}
      virtual unsigned Version() const {{ return VERSION_{version}; }}
      virtual void Run();{ccode}{write_post_call}
    }};

"""

TOKENS_SWITCH = """
  Log(ERR) << "Invalid {what} index: C{name}::{what}(" << idx << ")";
  throw ENotFound(EXCEPTION_MESSAGE);
"""

TOKENS_CARGUMENT = """
  return get_cargument(__FUNCTION__, idx, {args});
"""
TOKENS_HEADER_CCODE = '\n      virtual void Write(CCodeOStream &stream) const;'

TOKENS_SOURCE_CCODE = """


void gits::OpenCL::C{name}::Write(CCodeOStream &stream) const
{{
  {ccode}
}}"""

TOKENS_HEADER_WRITE_POST_CALL = '\n      virtual void WritePostCall(CCodeOStream &stream) const;'

TOKENS_SOURCE_WRITE_POST_CALL = """


void gits::OpenCL::C{name}::WritePostCall(CCodeOStream &stream) const
{{
  stream.Indent() << _return_value.WrapTypeNameStr() << "::AddMapping(0x" << (void*)_return_value.Original() << ", " << stream.VariableName(_return_value.ScopeKey()) << ");" << std::endl;
}}"""

TOKENS_SOURCE_FUNCTION = """

{separator}


{constructor}

gits::CArgument &gits::OpenCL::C{name}::Argument(unsigned idx)
{{{argument_impl}}}


gits::CArgument &gits::OpenCL::C{name}::Result(unsigned idx)
{{{result_impl}}}


void gits::OpenCL::C{name}::Run()
{{
  {run}
}}{ccode}{write_post_call}
"""

PLAYER_RUNWRAP_NAME = '../../OpenCL/common/include/openclPlayerRunWrap.h'

PLAYER_RUNWRAP_START = """
#pragma once

#include "gits.h"
#include "oclFunction.h"
#include "openclTools.h"
#include "openclArguments.h"

namespace gits {
namespace OpenCL {

"""

PLAYER_RUNWRAP_END = """
} // namespace OpenCL
} // namespace gits
"""

PLAYER_RUNWRAP_FUNCTION = """
    inline void {name}_RUNWRAP({args})
    {{
      {body}
    }}
"""

PLAYER_STATETRACK_NAME = '../../OpenCL/common/include/openclStateTracking.h'

PLAYER_STATETRACK_START = """
#pragma once

#include "gits.h"
#include "oclFunction.h"
#include "openclArguments.h"

namespace gits {
namespace OpenCL {

"""

PLAYER_STATETRACK_END = """
} // namespace OpenCL
} // namespace gits
"""

PLAYER_STATETRACK_FUNCTION = """
    inline void {name}_SD({args})
    {{

    }}
"""

RECORDER_SUBWRAPPERS_NAME = '../../OpenCL/recorder/include/openclRecorderSubwrappers.h'

RECORDER_SUBWRAPPERS_START = """
#pragma once

#include "openclFunctionsAuto.h"
#include "openclStateTracking.h"
#include "recorder.h"

namespace gits {
namespace OpenCL {
"""

RECORDER_SUBWRAPPERS_END = """
} // namespace OpenCL
} // namespace gits
"""

RECORDER_SUBWRAPPERS_FUNCTION = """
    inline void {name}_RECWRAP({args})
    {{

    }}
"""

IFDEF_PLATFORM = "#ifdef GITS_PLATFORM_"
IFNDEF_PLATFORM = "#ifndef GITS_PLATFORM_"
ENDIF_PLATFORM = "#endif"

ARGUMENTS_HEADER_NAME = 'openclArgumentsAuto.h'
ARGUMENTS_SOURCE_NAME = 'openclArgumentsAuto.cpp'

ARGUMENTS_HEADER_START = """
#pragma once

#include "openclHeader.h"
#include "openclTools.h"

#include "argument.h"

#include <string>
#include <vector>


namespace gits {
  namespace OpenCL {

    template<typename T, typename T_WRAP>
    class CCLArg : public CArgument {
    public:
      typedef T CLType;
      typedef T_WRAP GITSType;
    private:
      CLType _value;
    public:
      static const char *NAME;
      typedef CArgumentSizedArray<CLType, GITSType> CSArray;
      CCLArg() : _value() {};
      CCLArg(CLType &value) : _value(value) {};
      CLType &operator*() { return _value; }
      CLType &Original() { return _value; }
      virtual const char *Name() const { return NAME; }
      virtual void Write(CBinOStream& stream) const { write_name_to_stream(stream, _value); }
      virtual void Read(CBinIStream& stream) { read_name_from_stream(stream, _value); }
      virtual void Write(CCodeOStream& stream) const { stream << ToString(); }
      virtual std::string ToString() const { return ToStringHelper(Value()); }

      CLType &Value() { return _value; }
      const CLType &Value() const { return _value; }
    };

    template<class T, class T_WRAP>
    const char *gits::OpenCL::CCLArg<T, T_WRAP>::NAME = T_WRAP::NAME;

    template<class T, class T_WRAP>
    class CCLArgObj : public CArgument {
      T key_;
    public:
      static const char *NAME;
      typedef T CLType;
      typedef CArgumentMappedSizedArray<T, T_WRAP, gits::ADD_MAPPING> CSMapArray;
      typedef CArgumentMappedSizedArray<T, T_WRAP, gits::NO_ACTION> CSArray;

      CCLArgObj() = default;
      CCLArgObj(T arg) : key_(arg) {}
      CCLArgObj(T* arg) : key_(*arg) {}

      static void AddMapping(T key, T value) {
        get_map()[key] = value;
      }

      void AddMapping(T value) {
        AddMapping(key_, value);
      }

      static void AddMapping(const T* keys, const T* values, size_t num) {
        for (size_t i = 0; i < num; ++i)
          AddMapping(keys[i], values[i]);
      }

      void RemoveMapping() {
        RemoveMapping(key_);
      }

      static void RemoveMapping(T key) {
        if (CheckMapping(key))
          if (GetRefCount(GetMapping(key)) == 0)
            get_map().erase(key);
      }

      static void RemoveMapping(const T* keys, size_t num) {
        for (size_t i = 0; i < num; ++i)
          RemoveMapping(keys[i]);
      }

      static T GetMapping(T key) {
        if(key == nullptr) return nullptr;

        auto& the_map = get_map();
        auto iter = the_map.find(key);
        if (iter == the_map.end()) {
          Log(ERR) << "Couldn't map OpenCL object name " << key;
          throw std::runtime_error(EXCEPTION_MESSAGE);
        }
        return iter->second;
      }

      static void GetMapping(const T* keys, T* values, size_t num) {
        for (size_t i = 0; i < num; ++i)
          values[i] = GetMapping(keys[i]);
      }

      static std::vector<T> GetMapping(const T* keys, size_t num) {
        std::vector<T> v;
        v.reserve(num);
        for (size_t i = 0; i < num; ++i)
          v.push_back(GetMapping(keys[i]));
        return v;
      }

      static bool CheckMapping(T key) {
        auto& the_map = get_map();
        return the_map.find(key) != the_map.end();
      }

      static T GetOriginal(T value) {
        const auto &map = get_map();
        for (auto it = map.begin(); it != map.end(); ++it) {
          if (it->second == value) {
            return it->first;
          }
        }
        Log(ERR) << "Couldn't find the original OpenCL object " << value;
        throw std::runtime_error(EXCEPTION_MESSAGE);
      }

      bool CheckMapping() {
        return CheckMapping(key_);
      }

      virtual const char *Name() const { return NAME; }
      static const char* TypeNameStr() { return NAME; }
      static const char* WrapTypeNameStr() { return "CCLArgObj"; }

      void Reset(T value) { key_ = value; }
      T Original() const { return key_; }
      T Value() const { return GetMapping(key_); }
      T operator*() const { return Value(); }

      virtual void Write(CBinOStream& stream) const {
        write_name_to_stream(stream, key_);
      }

      virtual void Read(CBinIStream& stream) {
        read_name_from_stream(stream, key_);
      }

      virtual void Declare(CCodeOStream &stream) const {
        stream.Indent() << Name() << " " << stream.VariableName(ScopeKey()) << " = (" << TypeNameStr() << ") 0x" << (void*)key_ << ";" << std::endl;
      }

      virtual void Write(CCodeOStream& stream) const {
        stream << "(" << TypeNameStr() << ") " << WrapTypeNameStr() << "::GetMapping(0x" << (void*)key_ << ")";
      }

      void Assign(T other) { AddMapping(other); }

    private:
      typedef std::unordered_map<T, T> name_map_t;
      static name_map_t& get_map() {
        INIT_NEW_STATIC_OBJ(objects_map, name_map_t)
          static bool initialized = false;
        if (!initialized)
          objects_map[0] = 0;
        return objects_map;
      }
    };

    template<class T, class T_WRAP>
    const char *gits::OpenCL::CCLArgObj<T, T_WRAP>::NAME = T_WRAP::NAME;

"""

ARGUMENTS_HEADER_END = """
} // namespace OpenCL
} // namespace gits
"""

ARGUMENTS_SOURCE_START = """
#include "openclArgumentsAuto.h"

#include "openclHeader.h"

#include "log.h"
#include "tools.h"


namespace gits {
namespace OpenCL {
"""

ARGUMENTS_SOURCE_END = """
} // namespace OpenCL
} // namespace gits
"""

ARGUMENTS_TOSTRING_DECLARATION = '    std::string {}ToString(const {} value);'
ARGUMENTS_TOSTRING_DEFINITION = """
std::string {0}ToString(const {1} value)
{{{2}}}
"""
ARGUMENTS_TOSTRING_SWITCH = """
  switch(value) {{
{0}
  default: return ToStringHelper(reinterpret_cast<const void*>(value));
  }}
"""
ARGUMENTS_TOSTRING_CASE = '  case {}: return "{}";'

ARGUMENTS_TOSTRING_BITFIELD = """
  {0} res = 0;
  std::string text;
{1}
  if(res != value || value == 0) {{
    return std::to_string(value);
  }}
  return text;
"""
ARGUMENTS_TOSTRING_BITFIELD_ALL = """  if(value == 0xFFFFFFFF)
    return "CL_DEVICE_TYPE_ALL";
"""
ARGUMENTS_TOSTRING_BITFIELD_CASE = """  if(value & {0}) {{
    MaskAppend(text, "{1}");
    res |= {0};
  }}"""
ARGUMENTS_TOSTRING_BITFIELD_CASE_ZERO = """  if(value == {0}) {{
    MaskAppend(text, "{1}");
    res |= {0};
  }}"""

ARGUMENTS_ARGUMENT_DECLARATION = """
    class C{0} : public CCLArg{4}<{1}, C{0}> {{
    public:
      static const char* NAME;
      C{0}(): CCLArg{4}() {{}}
      C{0}(CLType value): CCLArg{4}(value) {{}}{2}{3}
    }};
"""

ARGUMENTS_ARGUMENT_DECLARATION_TOSTRING = """
      virtual std::string ToString() const {}"""
ARGUMENTS_ARGUMENT_DECLARATION_CCODE = """
      virtual void Write(CCodeOStream&) const override;"""

ARGUMENTS_ARGUMENT_NAME = 'const char *C{0}::NAME = "{1}";'

DEF_NAME = 'oclPlugin.def'
DEF_START = """
LIBRARY OpenCL.dll
EXPORTS
"""

HEADER_NAME = 'openclHeaderAuto.h'

LUA_CONSTANTS_NAME = 'clconstants.lua'
