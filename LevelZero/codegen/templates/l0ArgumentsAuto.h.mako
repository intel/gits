// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "l0Arguments.h"
#include "l0Log.h"

namespace gits {
  namespace l0 {
    template<typename StructureType, typename StructureBaseType>
    gits::CArgument *AllocateExtensionStructure(Cuint32_t *version, const StructureType &stype, StructureBaseType *extendedProperties = nullptr) {
      throw ENotImplemented(EXCEPTION_MESSAGE);
    };
%for name, enum in enums.items():

    class C${name} : public CArg<${enum.get('name')}, C${name}> {
    public:
      static const char* NAME;
      C${name}() = default;
      C${name}(L0Type value) : CArg(value) {}
      virtual std::string ToString() const { return ToStringHelper(Value()); }
      virtual void Write(CBinOStream& stream) const { CArg::Write(stream); }
    };
  %if "_structure_type_t" in name:
    template<>
    gits::CArgument *AllocateExtensionStructure(Cuint32_t *version, const ${name} &stype, ${get_namespace(name)}_base_properties_t *extendedProperties);
  %endif
%endfor

    template <typename StructureType, typename CStructureType, typename StructureBaseType>
    class CExtensionStruct : public CExtensionStructBase {
    private:
      CStructureType _stype{};
      Cuint32_t _version;
      CArgument *_pNext = nullptr;
    public:
      CExtensionStruct() = default;
      CExtensionStruct(void *value) {
        if (value == nullptr) {
          _stype = static_cast<StructureType>(0);
          return;
        }
        auto *extendedProperties = reinterpret_cast<StructureBaseType *>(value);
        _stype = extendedProperties->stype;
        _pNext = AllocateExtensionStructure(&_version, _stype.Value(), extendedProperties);
      }
      CExtensionStruct(const void *value) : CExtensionStruct(const_cast<void*>(value)) {};
      virtual ~CExtensionStruct() {
        if (_stype.Value() != static_cast<StructureType>(0U) &&
            _pNext != nullptr) {
          delete _pNext;
        }
      }
      CExtensionStruct(const CExtensionStruct &) = delete;
      CExtensionStruct &operator=(const CExtensionStruct &) = delete;
      CExtensionStruct(CExtensionStruct &&) = delete;
      CExtensionStruct &operator=(CExtensionStruct &&) = delete;
      virtual const char *Name() const { return "void *"; }
      virtual void Write(CBinOStream &stream) const {
        _stype.Write(stream);
        if (_stype.Value() != static_cast<StructureType>(0U)) {
          _version.Write(stream);
          _pNext->Write(stream);
        }
      }
      virtual void Read(CBinIStream &stream) {
        _stype.Read(stream);
        if (_stype.Value() != static_cast<StructureType>(0U)) {
          _version.Read(stream);
          _pNext = AllocateExtensionStructure<StructureType, StructureBaseType>(&_version, _stype.Value());
          _pNext->Read(stream);
        }
      }
      void *operator*() {
        if (_stype.Value() != static_cast<StructureType>(0U)) {
          return _pNext->GetPtrType();
        }
        return nullptr;
      }
      virtual std::set<uint64_t> GetMappedPointers() {
        return std::set<uint64_t>();
      }
      virtual CArgument* GetCPtrType() {
        return _pNext;
      }
    };
    using CExtensionStructCore = CExtensionStruct<ze_structure_type_t, Cze_structure_type_t, ze_base_properties_t>;
    using CExtensionStructSysman = CExtensionStruct<zes_structure_type_t, Czes_structure_type_t, zes_base_properties_t>;
    using CExtensionStructTools = CExtensionStruct<zet_structure_type_t, Czet_structure_type_t, zet_base_properties_t>;
    using CExtensionStructTracer = CExtensionStruct<zel_structure_type_t, Czel_structure_type_t, zel_base_properties_t>;
%for name, arg in arguments.items():
  %if not arg.get('enabled', True):
<% continue %>
  %endif
%if not arg.get('custom', False):
%if arg.get('obj', False):
    class C${name} : public CArgHandle<${arg.get('name')}, C${name}> {
    public:
      static const char* NAME;
      C${name}() = default;
      C${name}(L0Type value) : CArgHandle(value) {}
      C${name}(L0Type* value) : CArgHandle(value) {}
      static void AddMutualMapping(${arg.get('name')} key, ${arg.get('name')} value);
      static void RemoveMutualMapping(${arg.get('name')} key);
      virtual std::string ToString() const { return ToStringHelper(Value()); }
    };
%else:
    class C${name} : public CArgument {
    private:
      typedef ${arg.get('name')} L0Type;
      %for field in arg['vars']:
      ${get_field_type(field)} _${get_field_name(field)};
      %endfor
      L0Type _struct = {};
    public:
      typedef CL0StructArray<L0Type, C${name}> CSArray;
      static const char* NAME;
      C${name}();
      C${name}(const L0Type& value);
      C${name}(const L0Type* value);
      virtual const char* Name() const { return "${arg.get('name')}"; }
      virtual void Write(CBinOStream& stream) const;
      virtual void Read(CBinIStream& stream);
      L0Type operator*() {
        const auto *ptr = Ptr();
        if (ptr == nullptr) {
          throw EOperationFailed(EXCEPTION_MESSAGE);
        }
        return *ptr;
      }
      L0Type* Ptr();
      void * GetPtrType() override { return (void *)Ptr(); }

      virtual std::set<uint64_t> GetMappedPointers() { return std::set<uint64_t>(); }
    };
%endif
%endif
%endfor

  }
}
