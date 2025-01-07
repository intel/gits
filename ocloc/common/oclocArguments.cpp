// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "oclocArguments.h"

/***************** CPROGRAMSOURCES ******************/

unsigned gits::ocloc::CProgramSources::_programSourceIdx = 0;

std::string gits::ocloc::CProgramSources::GetFileName() {
  std::stringstream stream;
  stream << "oclocPrograms/kernel_source_" << std::setfill('0') << std::setw(2)
         << _programSourceIdx++;
  return stream.str();
}

gits::ocloc::CProgramSources::CProgramSources(uint32_t count,
                                              const char** strings,
                                              const size_t* lengths,
                                              const char** /*sourceNames*/) {
  for (uint32_t i = 0; i < count; ++i) {
    size_t length = strings[i][lengths[i] - 1] == '\0' ? lengths[i] - 1 : lengths[i];
    _files.emplace_back(GetFileName().c_str(), strings[i], length);
  }
}

const char** gits::ocloc::CProgramSources::Value() {
  static std::vector<const char*> value;
  value.resize(_files.size());
  for (size_t i = 0; i < _files.size(); ++i) {
    value[i] = _files[i].Text().c_str();
  }

  _sources_array = value.data();
  return _sources_array;
}

size_t* gits::ocloc::CProgramSources::Lengths() {
  static std::vector<size_t> lengths;
  lengths.resize(_files.size());
  for (size_t i = 0; i < _files.size(); ++i) {
    lengths[i] = _files[i].Text().size();
  }

  _sources_lengths_array = lengths.data();
  return _sources_lengths_array;
}

const char** gits::ocloc::CProgramSources::FileNames() {
  static std::vector<const char*> value;
  value.resize(_files.size());
  for (size_t i = 0; i < _files.size(); ++i) {
    value[i] = _files[i].FileName().c_str();
  }

  _filenames_array = value.data();
  return _filenames_array;
}

void gits::ocloc::CProgramSources::Write(CBinOStream& stream) const {
  write_name_to_stream(stream, _files.size());
  for (const auto& file : _files) {
    stream << file;
  }
}

void gits::ocloc::CProgramSources::Read(CBinIStream& stream) {
  size_t size = 0U;
  const auto ret = stream.read(reinterpret_cast<char*>(&size), sizeof(size));
  if (size > 0U && size <= UINT32_MAX && ret) {
    _files.resize(size);
    for (size_t i = 0; i < size; ++i) {
      stream >> _files[i];
    }
  }
}
