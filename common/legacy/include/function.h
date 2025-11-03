// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   function.h
 *
 * @brief Declaration of library function call wrapper.
 *
 */

#pragma once

#include "exception.h"
#include "token.h"
#include "library.h"
#include "pragmas.h"

#define GITS_BIT_AT_IDX(x) (1u << x)

namespace gits {
class CArgument;

/**
   * @brief Library function call wrapper
   *
   * gits::CFunction class contains unique identifier of the function,
   * its type and all its arguments. It stores function identifier, its type
   * and name. It also contains an array of function arguments. Class provides
   * Run() method that performs specific library call with proper arguments.
   */
class CFunction : public CToken {
public:
  virtual ~CFunction() = 0;

  virtual unsigned ArgumentCount() const = 0;
  virtual CArgument& Argument(unsigned idx) = 0;
  virtual const CArgument& Argument(unsigned idx) const;

  template <class T>
  T& Argument(unsigned idx) {
    return dynamic_cast<T&>(Argument(idx));
  }
  template <class T>
  const T& Argument(unsigned idx) const {
    return dynamic_cast<const T&>(Argument(idx));
  }

  virtual CArgument* Return();
  virtual const CArgument* Return() const = 0;
  template <class T>
  T& Return() {
    return dynamic_cast<T&>(*Return());
  }

  virtual unsigned ResultCount() const = 0;
  virtual CArgument& Result(unsigned idx) = 0;
  virtual const CArgument& Result(unsigned idx) const;
  template <class T>
  T& Result(unsigned idx) {
    return dynamic_cast<T&>(Result(idx));
  }

  /**
     * @brief Returns Id of a function
     *
     * Method returns identifier of a function.
     *
     * @return Id of a function
     */
  virtual unsigned Id() const = 0;

  /**
    * @brief Returns Id of a library
    *
    * Method returns identifier of a library.
    *
    * @return Id of a library
    */
  virtual CLibrary::TId LibraryId() const = 0;

  /**
     * @brief Returns name of a function
     *
     * Method returns name of a function.
     *
     * @return Name of a function
     */
  virtual const char* Name() const = 0;
  virtual const char* Suffix() const {
    return "";
  }

  /**
     * @brief Runs library function call.
     *
     * Method runs its library function with stored arguments.
     */
  virtual void Run() = 0;

  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
};

template <class... Args>
gits::CArgument& get_cargument(const char* func, unsigned idx, Args&... args);

namespace detail {
class UnswitchUnsafe {
  template <class... Args>
  friend gits::CArgument& gits::get_cargument(const char* func, unsigned idx, Args&... args);

#ifdef GITS_PLATFORM_WINDOWS
#pragma warning(push)
#pragma warning(disable : 4702)
#define ASSUME(x) __assume(x)
#else
#define ASSUME(x)
#endif
  template <int I>
  static inline void* perform(int n,
                              void* a0 = 0,
                              void* a1 = 0,
                              void* a2 = 0,
                              void* a3 = 0,
                              void* a4 = 0,
                              void* a5 = 0,
                              void* a6 = 0,
                              void* a7 = 0,
                              void* a8 = 0,
                              void* a9 = 0,
                              void* a10 = 0,
                              void* a11 = 0,
                              void* a12 = 0,
                              void* a13 = 0,
                              void* a14 = 0,
                              void* a15 = 0,
                              void* a16 = 0,
                              void* a17 = 0,
                              void* a18 = 0,
                              void* a19 = 0,
                              void* a20 = 0,
                              void* a21 = 0,
                              void* a22 = 0,
                              void* a23 = 0,
                              void* a24 = 0,
                              void* a25 = 0,
                              void* a26 = 0,
                              void* a27 = 0) {
#define CASE(x)                                                                                    \
  case x:                                                                                          \
    ASSUME(x < I);                                                                                 \
    return a##x;
    switch (n) {
      CASE(0);
      CASE(1);
      CASE(2);
      CASE(3);
      CASE(4);
      CASE(5);
      CASE(6);
      CASE(7);
      CASE(8);
      CASE(9);
      CASE(10);
      CASE(11);
      CASE(12);
      CASE(13);
      CASE(14);
      CASE(15);
      CASE(16);
      CASE(17);
      CASE(18);
      CASE(19);
      CASE(20);
      CASE(21);
      CASE(22);
      CASE(23);
      CASE(24);
      CASE(25);
      CASE(26);
      CASE(27);
    }
#undef CASE
    ASSUME(0);
    return 0;
  }
#ifdef GITS_PLATFORM_WINDOWS
#pragma warning(pop)
#else
#endif
#undef ASSUME
};
} // namespace detail

NORETURN void report_cargument_error(const char* func, unsigned idx);

template <class... Args>
gits::CArgument& get_cargument(const char* func, unsigned idx, Args&... args) {
  if (idx > sizeof...(args) - 1) {
    report_cargument_error(func, idx);
  }
  void* arg = detail::UnswitchUnsafe::perform<sizeof...(args)>(idx, &args...);
  if (arg == nullptr) {
    const auto msg = "Couldn't obtain cargument idx: " + std::to_string(idx) +
                     " of function: " + std::string(func);
    throw gits::EOperationFailed(std::string(EXCEPTION_MESSAGE) + msg);
  }
  return *(gits::CArgument*)arg;
}

} // namespace gits
