// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   exception.cpp
*
* @brief  GITS project exceptions definitions.
*
*/

#include "exception.h"
#include "log.h"
#include "tools_lite.h"
#include <typeinfo>
#include <map>
#include <vector>
#if defined GITS_PLATFORM_UNIX
#include <cxxabi.h>
#include <cstdlib>
#else
#endif
#include <cstdio>

#include "platform.h"
#if defined GITS_PLATFORM_WINDOWS
#include <Windows.h>
#include <psapi.h>
#endif

// Returns demangled name if possible or the unchanged name if demangling
// failed and status which is either the return code of __cxa_demangle or a -5
// if demangling is not supported on the current platform.
gits::DemanglingResult gits::TryToDemangle(const std::string mangledName) {
  DemanglingResult retVal;

#if defined GITS_PLATFORM_UNIX
  char* namePtr = abi::__cxa_demangle(mangledName.c_str(), nullptr, nullptr, &retVal.status);
  if (retVal.status == 0) {
    retVal.name = namePtr;
    free(namePtr);
  } else { // Error. Return the mangled name unchanged.
    retVal.name = mangledName;
  }
#else
  retVal.status = -5;
  retVal.name = mangledName;
#endif

  return retVal;
}

gits::Exception::FCallback gits::Exception::_callback = nullptr;
void* gits::Exception::_userData = nullptr;
bool gits::Exception::_inCallback = false;

void gits::Exception::Callback(FCallback callback, void* userData) {
  _callback = callback;
  _userData = userData;
}

gits::Exception::Exception() throw() {
  if (_callback && !_inCallback) {
    _inCallback = true;
    fprintf(stderr, "Err: Exception: %s\n", what());
    fprintf(stderr, "Err: Running user GITS exception callback\n");
    (*_callback)(_userData);
    _inCallback = false;
  }
}

gits::Exception::Exception(std::string message) throw() : _msg(std::move(message)) {
  if (_callback && !_inCallback) {
    _inCallback = true;
    fprintf(stderr, "Err: Exception: %s\n", what());
    fprintf(stderr, "Err: Running user GITS exception callback\n");
    (*_callback)(_userData);
    _inCallback = false;
  }
}

gits::Exception::~Exception() throw() {}

/**
* @brief Method returns a string describing current exception.
*
* @todo Maybe a stack trace of the place where an exception
*       was constructed should be gathered and printed.
*
* @return String describing the exception.
*/
const char* gits::Exception::what() const throw() {
  DemanglingResult result = TryToDemangle(typeid(*this).name());

  switch (result.status) {
  case 0:
    // Everything went fine.
    break;
  case -1:
    fprintf(stderr, "Err: Symbol demangling failed, couldn't allocate memory for the result.\n");
    break;
  case -2:
    fprintf(stderr,
            "Err: Symbol demangling failed, \"%s\" is not a valid name under the C++ ABI mangling "
            "rules.\n",
            result.name.c_str());
    break;
  case -3:
    fprintf(stderr, "Err: Symbol demangling failed, one of the arguments is invalid.\n");
    break;
  case -5:
    CALL_ONCE[] {
      fprintf(stderr, "Info: Couldn't demangle the name of the function the exception was thrown "
                      "in, GITS does not support symbol demangling on this platform.\n");
    };
    break;
  }

  _buffer = result.name + " at " + _msg;

  return _buffer.c_str();
}
namespace {
void fast_exit(int code) {
#if defined GITS_PLATFORM_WINDOWS
  _exit(code);
#else
  _Exit(code);
#endif
}
} // namespace

void topmost_exception_handler(const char* funcName) {
  try {
    throw;
  } catch (gits::Exception& ex) {
    fprintf(stderr, "Unhandled GITS exception: %s caught in: %s!!!\n", ex.what(), funcName);
  } catch (std::exception& ex) {
    fprintf(stderr, "Unhandled system exception: %s caught in: %s!!!\n", ex.what(), funcName);
  } catch (...) {
    fprintf(stderr, "Unhandled exception caught in: %s\n", funcName);
  }
  fast_exit(1);
}

#ifdef GITS_PLATFORM_WINDOWS

std::pair<std::string, std::string> DescribeException(DWORD exceptionCode) {
  using namespace gits;
  typedef std::map<DWORD, std::pair<std::string, std::string>> CExceptionMap;
  static CExceptionMap exceptionMap;
  if (exceptionMap.empty()) {
#define ADD_EXCEPTION_MAPPING(code, message)                                                       \
  exceptionMap[code] =                                                                             \
      std::make_pair<std::string, std::string>((const char*)#code, (const char*)message)
    ADD_EXCEPTION_MAPPING(EXCEPTION_ACCESS_VIOLATION,
                          "The thread tried to read from or write to a virtual address for which "
                          "it does not have the appropriate access.");
    ADD_EXCEPTION_MAPPING(EXCEPTION_ARRAY_BOUNDS_EXCEEDED,
                          "The thread tried to access an array element that is out of bounds and "
                          "the underlying hardware supports bounds checking.");
    ADD_EXCEPTION_MAPPING(EXCEPTION_BREAKPOINT, "A breakpoint was encountered.");
    ADD_EXCEPTION_MAPPING(
        EXCEPTION_DATATYPE_MISALIGNMENT,
        "The thread tried to read or write data that is misaligned on hardware that does not "
        "provide alignment. For example, 16-bit values must be aligned on 2-byte boundaries; "
        "32-bit values on 4-byte boundaries, and so on.");
    ADD_EXCEPTION_MAPPING(
        EXCEPTION_FLT_DENORMAL_OPERAND,
        "One of the operands in a floating-point operation is denormal. A denormal value is one "
        "that is too small to represent as a standard floating-point value.");
    ADD_EXCEPTION_MAPPING(
        EXCEPTION_FLT_DIVIDE_BY_ZERO,
        "The thread tried to divide a floating-point value by a floating-point divisor of zero.");
    ADD_EXCEPTION_MAPPING(EXCEPTION_FLT_INEXACT_RESULT,
                          "The result of a floating-point operation cannot be represented exactly "
                          "as a decimal fraction.");
    ADD_EXCEPTION_MAPPING(
        EXCEPTION_FLT_INVALID_OPERATION,
        "This exception represents any floating-point exception not included in this list.");
    ADD_EXCEPTION_MAPPING(EXCEPTION_FLT_OVERFLOW,
                          "The exponent of a floating-point operation is greater than the "
                          "magnitude allowed by the corresponding type.");
    ADD_EXCEPTION_MAPPING(
        EXCEPTION_FLT_STACK_CHECK,
        "The stack overflowed or underflowed as the result of a floating-point operation.");
    ADD_EXCEPTION_MAPPING(EXCEPTION_FLT_UNDERFLOW,
                          "The exponent of a floating-point operation is less than the magnitude "
                          "allowed by the corresponding type.");
    ADD_EXCEPTION_MAPPING(EXCEPTION_ILLEGAL_INSTRUCTION,
                          "The thread tried to execute an invalid instruction.");
    ADD_EXCEPTION_MAPPING(EXCEPTION_IN_PAGE_ERROR,
                          "The thread tried to access a page that was not present, and the system "
                          "was unable to load the page. For example, this exception might occur if "
                          "a network connection is lost while running a program over the network.");
    ADD_EXCEPTION_MAPPING(
        EXCEPTION_INT_DIVIDE_BY_ZERO,
        "The thread tried to divide an integer value by an integer divisor of zero.");
    ADD_EXCEPTION_MAPPING(EXCEPTION_INT_OVERFLOW,
                          "The result of an integer operation caused a carry out of the most "
                          "significant bit of the result.");
    ADD_EXCEPTION_MAPPING(
        EXCEPTION_INVALID_DISPOSITION,
        "An exception handler returned an invalid disposition to the exception dispatcher. "
        "Programmers using a high-level language such as C should never encounter this exception.");
    ADD_EXCEPTION_MAPPING(
        EXCEPTION_NONCONTINUABLE_EXCEPTION,
        "The thread tried to continue execution after a noncontinuable exception occurred.");
    ADD_EXCEPTION_MAPPING(EXCEPTION_PRIV_INSTRUCTION,
                          "The thread tried to execute an instruction whose operation is not "
                          "allowed in the current machine mode.");
    ADD_EXCEPTION_MAPPING(EXCEPTION_SINGLE_STEP,
                          "A trace trap or other single-instruction mechanism signaled that one "
                          "instruction has been executed.");
    ADD_EXCEPTION_MAPPING(EXCEPTION_STACK_OVERFLOW, "The thread used up its stack.");
#undef ADD_EXCEPTION_MAPPING
  }
  CExceptionMap::const_iterator it = exceptionMap.find(exceptionCode);
  std::pair<std::string, std::string> exceptionInfo;
  if (it == exceptionMap.end()) {
    std::stringstream str;
    str << exceptionCode;
    exceptionInfo.first = "Unrecognized exception code: " + str.str();
  } else {
    exceptionInfo = it->second;
  }
  return exceptionInfo;
}

std::string PointerModule(void* pointer) {
  using namespace gits;
  DWORD size = 0;

  HANDLE process = GetCurrentProcess();
  EnumProcessModules(process, 0, 0, &size);

  std::vector<HMODULE> modules(size / sizeof(HMODULE));
  EnumProcessModules(process, &modules[0], size, &size);

  Log(INFO) << "Exception address: " << pointer;
  Log(INFO) << "Modules loaded: " << modules.size();

  std::string faultyModule;
  for (auto module : modules) {
    MODULEINFO info;
    GetModuleInformation(process, module, &info, sizeof(MODULEINFO));
    void* beginOfModule = info.lpBaseOfDll;
    void* endOfModule = (char*)info.lpBaseOfDll + info.SizeOfImage;

    std::vector<char> buffer(2048, 0);
    GetModuleFileName(module, &buffer[0], 2048);

    Log(INFO) << "begin: " << beginOfModule << "  end: " << endOfModule << "  name: " << &buffer[0];
    if (pointer >= beginOfModule && pointer < endOfModule) {
      faultyModule = &buffer[0];
    }
  }

  return faultyModule.empty() ? "unknown" : faultyModule;
}

void gits::ShowExceptionInfo(_EXCEPTION_POINTERS* exceptionPtr) {
  PEXCEPTION_RECORD rec = exceptionPtr->ExceptionRecord;

  std::pair<std::string, std::string> info = DescribeException(rec->ExceptionCode);
  Log(ERR) << "Structured exception raised during GITS execution!"
           << "\n";
  Log(ERR) << "Exception code: " << info.first << "\n" << info.second << "\n";
  Log(ERR) << "Exception occured in module: " << PointerModule(rec->ExceptionAddress) << "\n";
}

#endif
