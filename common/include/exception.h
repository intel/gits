// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
* @file   exception.h
*
* @brief GITS project exceptions declarations.
*
*/

#pragma once

#include <sstream>
#include <stdexcept>
#include <string>

struct _EXCEPTION_POINTERS;

class CExceptionMessageInfo {
  const char* _function;
  const char* _file;
  int _line;

public:
  CExceptionMessageInfo(const char* function, const char* file, int line)
      : _function(function), _file(file), _line(line) {}
  operator std::string() const {
    std::stringstream ss;
    ss << _file << "(" << _line << "): " << _function;
    return ss.str();
  }
  std::string operator+(const char* cstr) const {
    return (std::string) * this + std::string(cstr);
  }
  std::string operator+(std::string str) {
    return (std::string) * this + str;
  }
  ~CExceptionMessageInfo() {}
};
#define EXCEPTION_MESSAGE CExceptionMessageInfo(__FUNCTION__, __FILE__, __LINE__)
/*workaround to quit compiler warning - C4702:unreachable code*/
#define UNREACHABLE_CODE_WA                                                                        \
  unsigned char result;                                                                            \
  memset(&result, 1, sizeof(result));                                                              \
  if (result == 1)

namespace gits {
struct DemanglingResult {
  int status;
  std::string name;
};
DemanglingResult TryToDemangle(const std::string mangledName);

std::string staticExceptionInfo(const char* func, const char* file, int line);

/**
  * @brief Base class for exceptions hierarchy.
  *
  * Class is a base for exceptions thrown by the GITS.
  */
class Exception : public std::exception {
public:
  typedef void (*FCallback)(void* userData);

private:
  static FCallback _callback;
  static void* _userData;
  static bool _inCallback;
  const std::string _msg;
  mutable std::string _buffer;

public:
  static void Callback(FCallback callback, void* userData);
  Exception() throw();
  Exception(std::string message) throw();
  Exception(const Exception& other) = delete;
  Exception& operator=(const Exception& other) = delete;
  ~Exception() throw();

  const char* what() const throw();
};

/**
  * @brief Operation failed exception.
  *
  * Operation failed exception.
  */
class EOperationFailed : public Exception {
public:
  EOperationFailed(const std::string& msg) : Exception(msg) {}
};

/**
  * @brief Object not found exception.
  *
  * Object not found exception.
  */
class ENotFound : public EOperationFailed {
public:
  ENotFound(const std::string& msg) : EOperationFailed(msg) {}
};

/**
  * @brief Operation not supported exception.
  *
  * Operation not supported exception.
  */
class ENotSupported : public EOperationFailed {
public:
  ENotSupported(const std::string& msg) : EOperationFailed(msg) {}
};

/**
  * @brief Object not initialized exception.
  *
  * Object not initialized exception.
  */
class ENotInitialized : public EOperationFailed {
public:
  ENotInitialized(const std::string& msg) : EOperationFailed(msg) {}
};

/**
  * @brief Feature not yet implemented exception.
  *
  *
  */
class ENotImplemented : public ENotSupported {
public:
  ENotImplemented(const std::string& msg) : ENotSupported(msg) {}
};

void ShowExceptionInfo(_EXCEPTION_POINTERS* exceptionPtr); //
} // namespace gits

void topmost_exception_handler(const char* funcName);
