// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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

namespace gits {
struct DemanglingResult {
  int status;
  std::string name;
};
DemanglingResult TryToDemangle(const std::string mangledName);

/**
  * @brief Base class for exceptions hierarchy.
  *
  * Class is a base for exceptions thrown by the GITS.
  */
class Exception : public std::exception {
  const std::string _msg;
  mutable std::string _buffer;

public:
  Exception() throw();
  Exception(std::string message) throw();

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
