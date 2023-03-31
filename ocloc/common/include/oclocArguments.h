// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "argument.h"

#include "exception.h"
#include "log.h"
#include "gits.h"
#include "config.h"
#include "pragmas.h"

namespace gits {
namespace ocloc {
class CProgramSources : public CArgument {
  static unsigned _programSourceIdx;
  const char** _sources_array;
  size_t* _sources_lengths_array;
  const char** _filenames_array;
  std::vector<CArgumentFileText> _files;

public:
  CProgramSources() {}
  CProgramSources(uint32_t count,
                  const char** strings,
                  const size_t* lengths,
                  const char** sourceNames);

  size_t* Lengths();
  const char** Value();
  const char** FileNames();

  virtual const char* Name() const {
    return "const char**";
  }
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& stream) const {
    stream << "outArg()";
  }

  struct PtrConverter {
  private:
    const char** _ptr;

  public:
    explicit PtrConverter(const char** ptr) : _ptr(ptr) {}
    operator const char*() const {
      return *_ptr;
    }
    operator const unsigned char*() const {
      return reinterpret_cast<const unsigned char*>(*_ptr);
    }
    operator const char**() const {
      return _ptr;
    }
    operator const unsigned char**() const {
      return reinterpret_cast<const unsigned char**>(_ptr);
    }
    operator const void*() const {
      return *_ptr;
    }
  };

  PtrConverter operator*() {
    return PtrConverter(Value());
  }

private:
  std::string GetFileName();
};
} // namespace ocloc
} // namespace gits
