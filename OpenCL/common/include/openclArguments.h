// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   openclArguments.h
 *
 * @brief Declarations of OpenCL library function call argument wrappers.
 */

#pragma once

#include "openclArgumentsAuto.h"

#include "exception.h"
#include "log.h"
#include "gits.h"
#include "config.h"
#include "pragmas.h"

namespace gits {
namespace OpenCL {

class CCallbackContext
    : public CCLArg<void(CL_CALLBACK*)(const char*, const void*, size_t, void*), CCallbackContext> {
public:
  static const char* NAME;
  static void CL_CALLBACK Callback(const char*, const void*, size_t, void*) {}
  CCallbackContext() : CCLArg() {}
  CCallbackContext(CLType value) : CCLArg(value) {}
  virtual const char* Name() const {
    return NAME;
  }
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& stream) const;
};

class CCallbackProgram : public CCLArg<void(CL_CALLBACK*)(cl_program, void*), CCallbackProgram> {
public:
  static const char* NAME;
  static void CL_CALLBACK Callback(cl_program, void*) {}
  CCallbackProgram() : CCLArg() {}
  CCallbackProgram(CLType value) : CCLArg(value) {}
  virtual const char* Name() const {
    return NAME;
  }
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& stream) const;
};

class CCallbackEvent : public CCLArg<void(CL_CALLBACK*)(cl_event, cl_int, void*), CCallbackEvent> {
public:
  static const char* NAME;
  static void CL_CALLBACK Callback(cl_event, cl_int, void*) {}
  CCallbackEvent() : CCLArg() {}
  CCallbackEvent(CLType value) : CCLArg(value) {}
  virtual const char* Name() const {
    return NAME;
  }
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& stream) const;
};

class CCallbackMem : public CCLArg<void(CL_CALLBACK*)(cl_mem, void*), CCallbackMem> {
public:
  static const char* NAME;
  static void CL_CALLBACK Callback(cl_mem, void*) {}
  CCallbackMem() : CCLArg() {}
  CCallbackMem(CLType value) : CCLArg(value) {}
  virtual const char* Name() const {
    return NAME;
  }
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& stream) const;
};

class CCallbackSVM
    : public CCLArg<void(CL_CALLBACK*)(cl_command_queue, cl_uint, void**, void*), CCallbackSVM> {
public:
  static const char* NAME;
  static void CL_CALLBACK Callback(cl_command_queue, cl_uint, void**, void*) {}
  CCallbackSVM() : CCLArg() {}
  CCallbackSVM(CLType value) : CCLArg(value) {}
  virtual const char* Name() const {
    return NAME;
  }
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& stream) const;
};

/* *************************** PROGRAMS ******************************** */

class CProgramSource : public CArgumentFileText {
  static unsigned _programSourceIdx;
  static unsigned _binarySourceIdx;
  const char* text_cstr = nullptr;
  size_t text_length = 0;

public:
  enum ProgramType {
    PROGRAM_SOURCE,
    PROGRAM_BINARY
  };
  CProgramSource() {}
  CProgramSource(cl_uint count, const char** strings, const size_t* lengths);
  CProgramSource(const unsigned char* binary, const size_t length);
  CProgramSource(const void* binary, const size_t length);

  size_t* Length();
  const char** Value();

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
    operator const void*() const {
      return *_ptr;
    }
  };

  PtrConverter operator*() {
    return PtrConverter(Value());
  }

private:
  std::string GetFileName(ProgramType type);
  std::string GetProgramSource(const char** strings, const cl_uint count, const size_t* lengths);
  std::string GetProgramBinary(const unsigned char* binary, const size_t length);
};

class CBinariesArray : public CArgument {
  std::vector<std::unique_ptr<CProgramSource>> _binaries;
  std::vector<const unsigned char*> _text;

public:
  CBinariesArray() {}
  CBinariesArray(const cl_uint count, const unsigned char** binaries, const size_t* lengths);

  virtual const char* Name() const {
    return "const unsigned char*";
  }
  virtual bool Array() const {
    return true;
  }
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& stream) const {
    stream << stream.VariableName(ScopeKey());
  }
  virtual void Declare(CCodeOStream& stream) const;
  virtual bool DeclarationNeeded() const {
    return true;
  }

  const unsigned char** Value();

  const unsigned char** operator*() {
    return Value();
  }
  virtual std::string ToString() const {
    return "binaries";
  }
};

class CBinariesArray_V1 : public CArgument {
  std::vector<std::unique_ptr<CProgramSource>> _binaries;
  std::vector<const uint8_t*> _text;
  ProgramBinaryLink _linkMode = ProgramBinaryLink::binary;
  cl_program _programOriginal = nullptr;
  std::vector<std::vector<uint8_t>> _binariesArray;

public:
  CBinariesArray_V1() = default;
  CBinariesArray_V1(const cl_uint& count, const uint8_t** binaries, const size_t* lengths);

  virtual const char* Name() const {
    return "const unsigned char*";
  }
  virtual bool Array() const {
    return true;
  }
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& stream) const {
    stream << stream.VariableName(ScopeKey());
  }
  virtual void Declare(CCodeOStream& stream) const;
  virtual bool DeclarationNeeded() const {
    return true;
  }
  cl_program GetProgramOriginal() const {
    return _programOriginal;
  }

  const unsigned char** Value();
  ProgramBinaryLink GetProgramBinaryLink() const {
    return _linkMode;
  }
  const unsigned char** operator*() {
    return Value();
  }
  virtual std::string ToString() const {
    return "binaries";
  }
  std::vector<std::string> FileNames() const;
};

/* *************************** POINTERS ******************************** */

class CvoidPtr : public CCLArg<void*, CvoidPtr> {
public:
  static const char* NAME;
  CvoidPtr() : CCLArg() {}
  CvoidPtr(CLType value) : CCLArg(value) {}
  virtual const char* Name() const {
    return NAME;
  }
  virtual void Write(CCodeOStream& stream) const;
  virtual std::string ToString() const override {
    return ToStringHelper(Value());
  }

  struct PtrConverter {
  private:
    void* _ptr;

  public:
    explicit PtrConverter(void* ptr) : _ptr(ptr) {}
#ifdef GITS_PLATFORM_WINDOWS
    operator ID3D10Buffer*() const {
      return static_cast<ID3D10Buffer*>(_ptr);
    }
    operator ID3D10Texture2D*() const {
      return static_cast<ID3D10Texture2D*>(_ptr);
    }
    operator ID3D10Texture3D*() const {
      return static_cast<ID3D10Texture3D*>(_ptr);
    }
    operator ID3D11Buffer*() const {
      return static_cast<ID3D11Buffer*>(_ptr);
    }
    operator ID3D11Texture2D*() const {
      return static_cast<ID3D11Texture2D*>(_ptr);
    }
    operator ID3D11Texture3D*() const {
      return static_cast<ID3D11Texture3D*>(_ptr);
    }
    operator IDirect3DSurface9*() const {
      return static_cast<IDirect3DSurface9*>(_ptr);
    }
#endif
    operator void*() const {
      return _ptr;
    }
  };

  PtrConverter operator*() {
    return PtrConverter(Value());
  }
};

class CCLMappedPtr : public CCLArgObj<void*, CCLMappedPtr> {
  CBinaryResource _data;
  bool _hasData = false;

public:
  static const char* NAME;
  CCLMappedPtr() = default;
  // onlyMap determines whether data will be dumped in this argument.
  // Data is dumped only in EnqueueUnmap, but we need to remember the mapped
  // pointer in EnqueueMap calls too.
  CCLMappedPtr(CLType value, bool onlyMap = true);
  CCLMappedPtr(cl_ulong* arg) : CCLArgObj(*reinterpret_cast<void**>(arg)) {}
  virtual const char* Name() const {
    return NAME;
  }
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& stream) const;
  bool HasData() const {
    return _hasData;
  }
  const CBinaryResource& Data() const {
    return _data;
  }
  void SyncBuffer();
};

class CCLKernelExecInfo : CCLArg<void*, CCLKernelExecInfo> {
private:
  std::unique_ptr<CCLMappedPtr::CSArray> _svmPtrs;
  Ccl_bool _fineGrainSystemParam;

public:
  CCLKernelExecInfo() {}
  /************************************************************************/
  /* Dependent on param_name param value could be a ptr to a cl_bool value*/
  /* or a void*[] type. The second one is an array of ptrs created        */
  /* using clSVMAlloc. */
  /************************************************************************/
  CCLKernelExecInfo(cl_kernel_exec_info param_name,
                    const void* param_value,
                    size_t param_value_size);
  static const char* NAME;
  virtual void* operator*();
  virtual const char* Name() const {
    return "void *";
  }
  virtual void Write(CCodeOStream& stream) const;
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
};

class CCLKernelExecInfo_V1 : CCLArg<void*, CCLKernelExecInfo_V1> {
private:
  KernelExecInfoType type = KernelExecInfoType::boolean;
  std::unique_ptr<CCLMappedPtr::CSArray> param_ptrs;
  Ccl_bool param_bool;
  Ccl_uint param_uint;

public:
  CCLKernelExecInfo_V1() {}
  CCLKernelExecInfo_V1(cl_kernel_exec_info param_name,
                       const void* param_value,
                       size_t param_value_size);
  static const char* NAME;
  virtual void* operator*();
  virtual const char* Name() const {
    return "void *";
  }
  virtual void Write(CCodeOStream& stream) const;
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
};

class CCLUserData : public CvoidPtr {
  static char _buffer;

public:
  CCLUserData() {}
  CCLUserData(CLType value) : CvoidPtr(value) {}
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& stream) const;
};

/* **************************** BUFFERS ******************************** */

class CBinaryData : public CArgument {
private:
  static const unsigned CODE_STREAM_INLINE_BUFFER_SIZE_MAX = 1024;
  static unsigned _lastId;
  mutable CBinaryResource _resource;

protected:
  size_t _size = 0U;
  void* _ptr = nullptr;
  std::vector<char> _buffer;

public:
  CBinaryData() : _size(0), _ptr(0) {}
  CBinaryData(const size_t size, const void* buffer);
  ~CBinaryData();
  virtual const char* Name() const {
    return "void*";
  }
  virtual size_t Length() const {
    return _size;
  }

  void* Value() {
    return !_buffer.empty() ? _buffer.data() : _ptr;
  }
  const void* Value() const {
    return !_buffer.empty() ? _buffer.data() : _ptr;
  }
  void* operator*() {
    return Value();
  }
  const void* operator*() const {
    return Value();
  }

  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& stream) const;
  virtual void Declare(CCodeOStream& stream) const;
  virtual bool DeclarationNeeded() const {
    return Length() > 0;
  }
  virtual std::string ToString() const override {
    return ToStringHelper(_ptr);
  }
  virtual void Deallocate();
};

class CKernelArgValue : public CBinaryData {
private:
  const void* _obj;

public:
  CKernelArgValue() : _obj(nullptr) {}
  CKernelArgValue(const size_t len, const void* buffer);
  const void* operator*();
  virtual void Write(CCodeOStream& stream) const;
  virtual bool DeclarationNeeded() const {
    return true;
  }
  virtual void Declare(CCodeOStream& stream) const;
};

class CKernelArgValue_V1 : public CBinaryData {
private:
  const void* _obj;

public:
  CKernelArgValue_V1() : _obj(nullptr){};
  CKernelArgValue_V1(const size_t len, const void* buffer);
  const void* operator*();
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& stream) const;
  virtual bool DeclarationNeeded() const {
    return true;
  }
  virtual void Declare(CCodeOStream& stream) const;
};

class CAsyncBinaryData : public CArgument {
protected:
  const void* _appPtr;
  size_t _len;
  CBinaryResource _resource;

public:
  CAsyncBinaryData() : _appPtr(0), _len(0) {}
  CAsyncBinaryData(uint64_t hash) : _appPtr(0), _len(0), _resource(hash) {}
  CAsyncBinaryData(size_t len, const void* appPtr, bool read = false);
  CAsyncBinaryData(const cl_image_format imageFormat,
                   const cl_image_desc imageDesc,
                   void* appPtr); //clCreateImage
  CAsyncBinaryData(const cl_image_format imageFormat,
                   size_t imageWidth,
                   size_t imageHeight,
                   size_t imageRowPitch,
                   void* appPtr); //clCreateImage2D
  CAsyncBinaryData(const cl_image_format imageFormat,
                   size_t imageWidth,
                   size_t imageHeight,
                   size_t imageDepth,
                   size_t imageRowPitch,
                   size_t imageSlicePitch,
                   void* appPtr); //clCreateImage3D
  CBinaryResource::PointerProxy operator*() {
    return Value();
  }
  CBinaryResource::PointerProxy Value();
  virtual const char* Name() const {
    return "void *";
  }
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& stream) const;
  virtual bool DeclarationNeeded() const {
    return _len > 0;
  }
  virtual void Declare(CCodeOStream& stream) const;
  virtual bool GlobalScopeVariable() const {
    return true;
  }
  virtual std::string VariableNamePrefix() const {
    return "async_buffer";
  }
  virtual intptr_t ScopeKey() const {
    return reinterpret_cast<intptr_t>(_appPtr);
  }
  virtual void VariableNameRegister(CCodeOStream& stream, bool returnValue) const;
  virtual std::string ToString() const override {
    return ToStringHelper(_appPtr);
  }
  void FreeHostMemory() {
    _resource.Deallocate();
  }
  bool IsMappedPointer() const {
    return _resource.GetResourceHash() == CResourceManager::EmptyHash;
  }
};

class CSVMPtr : CCLArg<void*, CSVMPtr> {
private:
  bool _createdByCLSVMAlloc = false;
  CCLMappedPtr _mappedPtr;
  CAsyncBinaryData _hostPtr;

public:
  CSVMPtr() {}
  /************************************************************************/
  /* If createdByCLSVMAlloc is true then ptr was created using clSVMAlloc */
  /* That means we need to use CCLMappedPtr for as the function argument  */
  /* That also means _hostPtr will be created from nullptr using size 0   */
  /* In case if token is using normal host ptr as a token argument then   */
  /* CCLMappedPtr will not be used (assigned as nullptr) and standard host*/
  /* ptr should have valid user data                                      */
  /************************************************************************/
  CSVMPtr(void* ptr, size_t len, bool createdByCLSVMAlloc);
  static const char* NAME;
  virtual void* operator*();
  virtual const char* Name() const {
    return "void *";
  }
  virtual void Write(CCodeOStream& stream) const;
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual std::string ToString() const override {
    return _createdByCLSVMAlloc ? ToStringHelper(*_mappedPtr) : _hostPtr.ToString();
  }
};

class CSVMPtr_V1 : CCLArg<void*, CSVMPtr_V1> {
private:
  bool _createdByCLSVMAlloc = false;
  CCLMappedPtr _mappedPtr;
  CBinaryData _hostPtr;
  uintptr_t _offset = 0;

public:
  CSVMPtr_V1() {}
  CSVMPtr_V1(void* ptr, size_t len);
  static const char* NAME;
  virtual void* operator*();
  virtual const char* Name() const {
    return "void *";
  }
  virtual void Write(CCodeOStream& stream) const;
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual std::string ToString() const override {
    return _createdByCLSVMAlloc ? ToStringHelper(*_mappedPtr) : _hostPtr.ToString();
  }
  void FreeHostMemory() {
    _hostPtr.Deallocate();
  }
  bool IsMappedPointer() {
    return _createdByCLSVMAlloc;
  }
};

std::vector<cl_context_properties> MapContextProperties(
    const Ccl_context_properties::CSArray& props);
std::vector<cl_mem_properties_intel> MapMemPropertiesIntel(
    const Ccl_mem_properties_intel::CSArray& props);

class CBuildOptions : public Cchar::CSArray {
private:
  bool _hasHeaders = false;

public:
  CBuildOptions() = default;
  CBuildOptions(const char* array, bool hasHeaders)
      : Cchar::CSArray(array, 0, 1), _hasHeaders(hasHeaders){};
  void Declare(CCodeOStream& stream) const;
};

class CGetContextInfoOutArgument : public CBinaryData {
private:
  bool _isPostActionNeeded = false;

public:
  CGetContextInfoOutArgument() = default;
  CGetContextInfoOutArgument(const size_t size, const void* buffer, cl_context_info param_info);
  virtual bool PostActionNeeded() const {
    return _isPostActionNeeded;
  }
  virtual void PostAction(CCodeOStream& stream) const;
  virtual void Declare(CCodeOStream& stream) const;
};

class CUSMPtr : CCLArg<void*, CUSMPtr> {
private:
  bool _createdByCLUSMAlloc = false;
  CBinaryData _hostPtr;
  CCLMappedPtr _mappedPtr;
  uintptr_t _offset = 0;
  void SetMappedOffset(void* ptr);

public:
  CUSMPtr() {}
  CUSMPtr(void* ptr, size_t len);
  //handling ptrs of usm ptr width and out of bound
  CUSMPtr(const void* ptr);
  static const char* NAME;
  void* Value();
  virtual void* operator*() {
    return Value();
  }
  virtual const char* Name() const {
    return "void *";
  }
  virtual void Write(CCodeOStream& stream) const;
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual std::string ToString() const override {
    return _createdByCLUSMAlloc ? ToStringHelper(*_mappedPtr) : _hostPtr.ToString();
  }
  virtual bool IsMappedPointer() {
    return _createdByCLUSMAlloc;
  }
  virtual void FreeHostMemory() {
    _hostPtr.Deallocate();
  }
};

class Ccl_resource_barrier_descriptor_intel : public CArgument {
private:
  typedef cl_resource_barrier_descriptor_intel CLType;
  CCLMappedPtr _svm_allocation_pointer;
  Ccl_mem _mem_object;
  Ccl_resource_barrier_type _type;
  Ccl_resource_memory_scope _scope;
  CLType _struct = {};

public:
  typedef CStructArray<CLType, Ccl_resource_barrier_descriptor_intel> CSArray;
  static const char* NAME;
  Ccl_resource_barrier_descriptor_intel(){};
  Ccl_resource_barrier_descriptor_intel(const CLType& value);
  Ccl_resource_barrier_descriptor_intel(const CLType* value)
      : Ccl_resource_barrier_descriptor_intel(*value) {}
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& stream) const;
  CLType Original() {
    return *Ptr();
  }
  CLType operator*() {
    return *Ptr();
  }
  CLType* Ptr();
  virtual const char* Name() const {
    return "cl_resource_barrier_descriptor_intel";
  }
  virtual std::string ToString() const override;
  virtual std::set<uint64_t> GetMappedPointers() {
    return std::set<uint64_t>();
  }
};

} // namespace OpenCL
} // namespace gits
