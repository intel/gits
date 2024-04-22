// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   argument.h
 *
 * @brief Declarations of library function call argument wrappers.
 *
 */

#pragma once

#include "log.h"
#include "gits.h"
#include "apis_iface.h"
#include "exception.h"
#include "streams.h"
#include "resource_manager.h"
#include "buffer.h"
#include "exception.h"

#include <vector>
#include <map>
#include <memory>

namespace gits {
class CBinOStream;
class CBinIStream;
class CCodeOStream;

/**
   * @brief Abstract base class for function call arguments wrappers
   *
   * gits::CArgument is an abstract base class for function call arguments
   * wrappers. Class provides an interface for saving and loading of arguments
   * data.
   */
class CArgument {
public:
  /**
     * @brief Returns the name of an argument
     *
     * Method returns the name of an argument.
     *
     * @return Name of an argument
     */
  virtual const char* Name() const = 0;

  virtual void Fill(void* ptr) const;

  /**
     * @brief Saves argument data to a binary file
     *
     * Method saves argument data to a binary file.
     *
     * @param stream Output stream to use.
     */
  virtual void Write(CBinOStream& stream) const = 0;

  /**
     * @brief Loads argument data from a binary file
     *
     * Method loads argument data from a binary file.
     *
     * @param stream Input stream to use.
     */
  virtual void Read(CBinIStream& stream) = 0;

  /**
     * @brief Saves argument data to a C code file
     *
     * Method saves argument data to a C code file.
     *
     * @param stream Output stream to use.
     */
  virtual void Write(CCodeOStream& stream) const = 0;

  /**
     * @brief Whether the output of Write needs to be prefixed with '&'
     *
     * If function expects a ptr and the argument we write is not a pointer, we
     * need to add an ampersand. For example if function expects
     * VkPresentInfoKHR* and we have a VkPresentInfoKHR then this method will
     * return true. But if the the argument is a nullptr, the ampersand isn't
     * needed (as nullptr is a pointer) and it will return false. It's used in
     * Vulkan CCode.
     */
  virtual bool AmpersandNeeded() const {
    return false;
  }
  virtual bool DeclarationNeeded() const {
    return false;
  }
  virtual void DeclareValue(CCodeOStream& stream) const {
    stream.VariableName(ScopeKey());
  }
  virtual void Declare(CCodeOStream& stream) const {
    stream.Indent() << Name() << " " << stream.VariableName(ScopeKey()) << " = ";
    Write(stream);
    stream << ";" << std::endl;
  }
  virtual bool PostActionNeeded() const {
    return false;
  }
  virtual void PostAction(CCodeOStream& /*stream*/) const {}
  virtual void* GetPtrType() {
    throw ENotImplemented(EXCEPTION_MESSAGE);
  }
  virtual bool GlobalScopeVariable() const {
    return false;
  }
  static bool InitializedWithOriginal() {
    return false;
  }
  virtual intptr_t ScopeKey() const {
    return reinterpret_cast<intptr_t>(this);
  }
  virtual std::string VariableNamePrefix() const {
    return Name();
  }
  virtual void VariableNameRegister(CCodeOStream& stream, bool returnValue) const {
    if (GlobalScopeVariable()) {
      stream.Register(ScopeKey(), VariableNamePrefix(), true, GlobalScopeVariable());
    } else {
      // only arrays have sense to be declared for local scope as variables so that must be a return value
      stream.Register(ScopeKey(), returnValue ? "ret" : "var", !returnValue);
    }
  }

  virtual std::string ToString() const {
    return "";
  }

  virtual ~CArgument() {}
};

inline CBinOStream& operator<<(CBinOStream& stream, const CArgument& argument) {
  argument.Write(stream);
  return stream;
}

inline CBinIStream& operator>>(CBinIStream& stream, CArgument& argument) {
  argument.Read(stream);
  return stream;
}

namespace {
template <typename T>
struct use_write {
  static const bool value = std::is_base_of<CArgument, T>::value || std::is_base_of<hex, T>::value;
};

template <typename T, typename std::enable_if<use_write<T>::value, bool>::type = true>
void stream_output(CCodeOStream& s, const T& t) {
  t.Write(s);
}

// stream argument cannot be CCodeOStream because it will create infinite loop
template <typename T, typename std::enable_if<!use_write<T>::value, bool>::type = true>
void stream_output(std::ostream& s, const T& t) {
  s << t;
}
} // namespace

/**
   * @brief C code dumper of function argument
   *
   * Method that saves function argument data to a C code file.
   * It uses Write method for CArgument derived classes and hex helper,
   * and std::operator<< for other types, preserving CCodeStream as return type.
   *
   * @param stream Output stream to use
   * @param argument Function argument to use
   *
   * @return Output stream
   */
template <typename T>
gits::CCodeOStream& operator<<(gits::CCodeOStream& stream, const T& t) {
  stream_output(stream, t);
  return stream;
}

/**
   * @brief Argument wrapper class for handling binary files
   *
   * gits::CArgumentBuffer class is a wrapper that is responsible
   * for writing and reading argument data from a binary file.
   * It gets a pointer to a buffer of binary argument data and its
   * length in a constructor. That buffer is then used to save/load
   * binary representation of that argument to a binary file. Sequential
   * bytes are ordered in opposite byte order than the network byte order.
   */
class CArgumentBuffer : public CArgument {
public:
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual char* Buffer() = 0;
  virtual const char* Buffer() const = 0;
  virtual size_t Length() const = 0;
};

class CArgumentFileText : public CArgument {
  std::string _fileName;
  std::string _text;

public:
  CArgumentFileText() {}
  CArgumentFileText(const std::string& fileName, const std::string& text);
  CArgumentFileText(const char* fileName, const char* text, unsigned length);
  virtual const char* Name() const {
    return _fileName.c_str();
  }
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& stream) const;

  const std::string& FileName() const {
    return _fileName;
  }
  const std::string& Text() const {
    return _text;
  }

  std::string& operator*() {
    return _text;
  }
  const std::string& operator*() const {
    return _text;
  }

private:
  void init(const char* fileName, const char* text, unsigned length);
};

class CBinaryResource : public CArgument {
public:
  struct PointerProxy {
    PointerProxy() : _ptr(0) {}
    explicit PointerProxy(const void* ptr) : _ptr(ptr) {}
    explicit PointerProxy(const mapped_file& data) : _data(data), _ptr(0) {}
    template <class T>
    operator const T*() const {
      return _ptr ? (const T*)_ptr : (const T*)_data.address();
    }
    operator const void*() const {
      return _ptr ? _ptr : _data.address();
    }
    size_t Size() const {
      return _data.size();
    }

  private:
    mapped_file _data;
    const void* _ptr;
  };

  explicit CBinaryResource();
  explicit CBinaryResource(hash_t hash);
  CBinaryResource(TResourceType type, const void* data, size_t size);

  void reset(TResourceType type, const void* data, size_t size);

  virtual const char* Name() const;
  virtual bool Array() const;

  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& stream) const;

  PointerProxy Data() const;
  PointerProxy operator*() {
    return Data();
  }
  PointerProxy Original() {
    return Data();
  }

  hash_t GetResourceHash() const;

protected:
  hash_t _resource_hash;
  mapped_file _data;
};

/**
   * @brief Wrapper for an array of arguments
   *
   * gits::CArgumentArray is a wrapper for an array of function call
   * arguments. It contains an array of library arguments and a second
   * array of wrappers for them.
   */
template <class T, int N, class T_WRAP>
class CArgumentFixedArray : public CArgument {
  T _array[N]; /**< @brief an array of library arguments */
  typedef T_WRAP CGLtype;

protected:
  typedef T_WRAP CWrapType;
  void Declare(CCodeOStream& stream, const std::string& declString) const;

public:
  CArgumentFixedArray() {
    for (int i = 0; i < N; i++) {
      _array[i] = 0;
    }
  }
  CArgumentFixedArray(const T array[]) {
    for (int i = 0; i < N; i++) {
      _array[i] = array[i];
    }
  }
  T* operator*() {
    return _array;
  }
  const T* operator*() const {
    return _array;
  }

  virtual const void* Data() const {
    return _array;
  }
  virtual const char* Name() const {
    return T_WRAP::NAME;
  }
  virtual bool Array() const {
    return true;
  }
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual int Size() const {
    return N;
  }
  virtual void Write(CCodeOStream& stream) const;
  virtual void Declare(CCodeOStream& stream) const;
  virtual bool DeclarationNeeded() const {
    return true;
  }
  virtual void DeclareElement(unsigned idx, CCodeOStream& stream) const {
    CWrapType wrap(_array[idx]);
    wrap.DeclareValue(stream);
  }
};

/**
   * @brief Wrapper for an array of arguments
   *
   * gits::CArgumentSizedArrayBase is a wrapper for an array of function call
   * arguments. Array is aware of number of elements it contains.
   * This parameter is written to the stream
   */
template <class T, class T_WRAP>
class CArgumentSizedArrayBase : public CArgument {
  std::vector<T> _array;
  typedef T_WRAP CGLtype;

public:
  CArgumentSizedArrayBase(size_t num = 0);
  CArgumentSizedArrayBase(size_t num, const T* array);
  CArgumentSizedArrayBase(const std::vector<T>& array);
  // Expect pointer to 'terminator' ended array, ignore terminators that
  // aren't at a position that is a multiple of term_pos.
  CArgumentSizedArrayBase(const T* array, T terminator, int term_pos);

  std::vector<T>& Vector() {
    return _array;
  }
  const std::vector<T>& Vector() const {
    return _array;
  }

  virtual bool Array() const {
    return true;
  }
  T* operator*() {
    if (_array.empty()) {
      return nullptr;
    }
    return _array.data();
  }
  const T* operator*() const {
    if (_array.empty()) {
      return nullptr;
    }
    return _array.data();
  }

  // Assign method is do nothing for non mapped arguments
  void Assign(const T* array) {}

  virtual const char* Name() const {
    return T_WRAP::NAME;
  }
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& stream) const;
  virtual void Declare(CCodeOStream& stream) const;
  virtual bool DeclarationNeeded() const {
    return true;
  }
};

/**
  * @brief Wrapper for an array of arguments
  *
  * gits::CArgumentSizedArray is divided using partial template specialization into
  * default CArgumentSizedArray<T, T_WRAP> and CArgumentSizedArray<char, T_WRAP>
  * We made this split because of CCode char* readability (previously it was table of int's)
  */
template <class T, class T_WRAP, class T_GET_MAPPED_POINTERS = uint64_t>
class CArgumentSizedArray : public CArgument {
  CArgumentSizedArrayBase<T, T_WRAP> _sizedArray;
  typedef T_WRAP CGLtype;

public:
  CArgumentSizedArray(size_t num = 0) : _sizedArray(num) {}
  template <size_t N>
  CArgumentSizedArray(const T (&array)[N]) : _sizedArray(N, array) {}
  CArgumentSizedArray(size_t num, const T* array) : _sizedArray(num, array) {}
  CArgumentSizedArray(const std::vector<T>& array) : _sizedArray(array) {}
  CArgumentSizedArray(const T* array, T terminator, int term_pos)
      : _sizedArray(array, terminator, term_pos) {}
  CArgumentSizedArray(const T* ptr) : _sizedArray(1, ptr) {}
  virtual intptr_t ScopeKey() const {
    return _sizedArray.ScopeKey();
  }
  virtual void DeclareValue(CCodeOStream& stream) const {
    _sizedArray.DeclareValue(stream);
  }
  virtual bool PostActionNeeded() const {
    return _sizedArray.PostActionNeeded();
  }
  virtual void PostAction(CCodeOStream& stream) const {
    _sizedArray.PostAction(stream);
  }

  virtual bool GlobalScopeVariable() const {
    return _sizedArray.GlobalScopeVariable();
  }

  virtual std::string VariableNamePrefix() const {
    return _sizedArray.VariableNamePrefix();
  }
  virtual void VariableNameRegister(CCodeOStream& stream, bool returnValue) const {
    _sizedArray.VariableNameRegister(stream, returnValue);
  }

  virtual std::string ToString() const {
    return _sizedArray.ToString();
  }

  std::vector<T>& Vector() {
    return _sizedArray.Vector();
  }
  const std::vector<T>& Vector() const {
    return _sizedArray.Vector();
  }

  virtual bool Array() const {
    return _sizedArray.Array();
  }
  T* operator*() {
    return _sizedArray.operator*();
  }
  const T* operator*() const {
    return _sizedArray.operator*();
  }
  T* Original() {
    return _sizedArray.operator*();
  }
  const T* Original() const {
    return _sizedArray.operator*();
  }

  // Assign method is do nothing for non mapped arguments
  void Assign(const T* array) {
    _sizedArray.Assign(array);
  }

  virtual const char* Name() const {
    return _sizedArray.Name();
  }
  virtual void Write(CBinOStream& stream) const {
    _sizedArray.Write(stream);
  }
  virtual void Read(CBinIStream& stream) {
    _sizedArray.Read(stream);
  }
  virtual void Write(CCodeOStream& stream) const {
    _sizedArray.Write(stream);
  };
  virtual void Declare(CCodeOStream& stream) const {
    _sizedArray.Declare(stream);
  }
  virtual bool DeclarationNeeded() const {
    return _sizedArray.DeclarationNeeded();
  }
  virtual std::set<T_GET_MAPPED_POINTERS> GetMappedPointers() {
    return std::set<T_GET_MAPPED_POINTERS>();
  }
};

template <class T_WRAP>
class CArgumentSizedArray<char, T_WRAP> : public CArgument {
  CArgumentSizedArrayBase<char, T_WRAP> _sizedArray;
  typedef T_WRAP CGLtype;

public:
  CArgumentSizedArray(size_t num = 0) : _sizedArray(num) {}
  template <size_t N>
  CArgumentSizedArray(const char (&array)[N]) : _sizedArray(N, array) {}
  CArgumentSizedArray(size_t num, const char* array) : _sizedArray(num, array) {}
  CArgumentSizedArray(const std::vector<char>& array) : _sizedArray(array) {}
  CArgumentSizedArray(const char* array, char terminator, int term_pos)
      : _sizedArray(array, terminator, term_pos) {}
  CArgumentSizedArray(const char* array) : _sizedArray(array, 0, 1) {}
  virtual intptr_t ScopeKey() const {
    return _sizedArray.ScopeKey();
  }
  virtual void DeclareValue(CCodeOStream& stream) const {
    _sizedArray.DeclareValue(stream);
  }
  virtual bool PostActionNeeded() const {
    return _sizedArray.PostActionNeeded();
  }
  virtual void PostAction(CCodeOStream& stream) const {
    _sizedArray.PostAction(stream);
  }

  virtual bool GlobalScopeVariable() const {
    return _sizedArray.GlobalScopeVariable();
  }

  virtual std::string VariableNamePrefix() const {
    return _sizedArray.VariableNamePrefix();
  }
  virtual void VariableNameRegister(CCodeOStream& stream, bool returnValue) const {
    _sizedArray.VariableNameRegister(stream, returnValue);
  }

  virtual std::string ToString() const {
    if (Vector().empty()) {
      return "";
    }
    const auto ret = **this;
    if (ret == nullptr) {
      const auto msg = "The pointer should only be null when the array is empty.";
      throw std::logic_error(std::string(EXCEPTION_MESSAGE) + msg);
    } else {
      return std::string(ret);
    }
  }

  std::vector<char>& Vector() {
    return _sizedArray.Vector();
  }
  const std::vector<char>& Vector() const {
    return _sizedArray.Vector();
  }

  virtual bool Array() const {
    return _sizedArray.Array();
  }
  char* operator*() {
    return _sizedArray.operator*();
  }
  const char* operator*() const {
    return _sizedArray.operator*();
  }

  char* Original() {
    return _sizedArray.operator*();
  }
  const char* Original() const {
    return _sizedArray.operator*();
  }

  // Assign method is do nothing for non mapped arguments
  void Assign(const char* array) {
    _sizedArray.Assign(array);
  }

  virtual const char* Name() const {
    return _sizedArray.Name();
  }
  virtual void Write(CBinOStream& stream) const {
    _sizedArray.Write(stream);
  }
  virtual void Read(CBinIStream& stream) {
    _sizedArray.Read(stream);
  }
  virtual void Write(CCodeOStream& stream) const {
    _sizedArray.Write(stream);
  };
  virtual void Declare(CCodeOStream& stream) const;
  virtual bool DeclarationNeeded() const {
    return _sizedArray.DeclarationNeeded();
  }
  virtual std::set<uint64_t> GetMappedPointers() {
    return std::set<uint64_t>();
  }
};

/**
  * @brief Wrapper for an array of arguments
  *
  * gits::CArgumentMappedSizedArray is a wrapper for an array of mapped function call
  * arguments. Array is aware of number of elements it contains. Mapped array returns unmapped values.
  * If accessed as non const mappings are being updated
  * This parameter is written to the stream
  */
enum MappedArrayAction { ADD_MAPPING = 0, REMOVE_MAPPING = 1, NO_ACTION = 2 };

template <class T, class T_WRAP, MappedArrayAction T_ACTION>
class CArgumentMappedSizedArray : public CArgument {

  enum ValuesType {
    MAPPEDS = 0,
    ORIGS,
    NULLS,
  };
  virtual void WriteArray(CCodeOStream& stream, std::string name, ValuesType valtype) const;
  virtual void WritePartArray(
      CCodeOStream& stream, std::string name, ValuesType valtype, size_t start, size_t end) const;

  typedef T_WRAP CGLtype;

public:
  MappedArrayAction Action() const {
    return T_ACTION;
  }
  std::vector<T> _array;
  std::vector<T> _mappedArray;
  std::vector<T> _array_T_original;

  struct ProxyArray {
    std::vector<T>* _mappedArray;
    std::vector<T>* _array;
    MappedArrayAction _action;
    typedef T_WRAP CGLtype;

    ProxyArray(const ProxyArray& other) = delete;
    ProxyArray& operator=(const ProxyArray& other) = delete;
    ProxyArray(std::vector<T>& arr, std::vector<T>& mappedarr, MappedArrayAction action)
        : _mappedArray(&mappedarr), _array(&arr), _action(action) {}
    ~ProxyArray() {
      try {
        if (_action == ADD_MAPPING) {
          // Add mapping
          for (size_t i = 0; i < _array->size(); i++) {
            CGLtype::AddMapping((*_array)[i], (*_mappedArray)[i]);
          }
        }
      } catch (...) {
        topmost_exception_handler("ProxyArray::~ProxyArray");
      }
    }
    operator T*() {
      // Return current mappings or default values (zeros)
      if (_array->empty()) {
        return nullptr;
      }
      auto size = _array->size();
      _mappedArray->resize(size);
      for (decltype(size) i = 0; i < size; i++) {
        auto& elem = (*_array)[i];
        // if it is not ADD_MAPPING mode map GetMapping should crash if element
        // is not mapped
        if (CGLtype::CheckMapping(elem) || _action != ADD_MAPPING) {
          (*_mappedArray)[i] = CGLtype::GetMapping(elem);
        } else if (CGLtype::InitializedWithOriginal()) {
          (*_mappedArray)[i] = elem;
        }
      }
      return _mappedArray->data();
    }
  };

  CArgumentMappedSizedArray(size_t num = 0);
  CArgumentMappedSizedArray(size_t num, const T* array);
  CArgumentMappedSizedArray(const std::vector<T>& array);
  // expect pointer to 'terminator' ended array, consider values to be
  // terminator only when it has specific value and is at position 'i == 0 %
  // term_pos'
  CArgumentMappedSizedArray(const T* array, T terminator, int term_pos);
  CArgumentMappedSizedArray(const T* array);

  virtual bool Array() const {
    return true;
  }
  ProxyArray operator*() {
    return ProxyArray(_array, _mappedArray, Action());
  }
  const ProxyArray operator*() const {
    return ProxyArray(_array, _mappedArray, Action());
  }
  T* Original() {
    if (_array.size() == 0) {
      return 0;
    }

    if (_array_T_original.size() == 0) {
      _array_T_original.resize(_array.size());
      for (size_t i = 0; i < _array.size(); i++) {
        _array_T_original[i] = T(_array[i]);
      }
    }
    return &_array_T_original[0];
  }
  size_t Size() const {
    return _array.size();
  }
  void RemoveMapping() {
    for (size_t i = 0; i < _array.size(); i++) {
      CGLtype::RemoveMapping(_array[i]);
    }
  }

  std::vector<T>& Vector() {
    return _array;
  }
  const std::vector<T>& Vector() const {
    return _array;
  }

  virtual const char* Name() const {
    return CGLtype::NAME;
  }
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);
  virtual void Write(CCodeOStream& stream) const;
  virtual void Declare(CCodeOStream& stream) const;
  virtual void Declare(CCodeOStream& stream, size_t start, size_t end) const;
  virtual bool DeclarationNeeded() const {
    return true;
  }
  virtual void PostAction(CCodeOStream& stream) const;
  virtual void PostAction(CCodeOStream& stream, size_t start, size_t end) const;
  virtual bool PostActionNeeded() const {
    return true;
  }
  virtual std::set<T> GetMappedPointers() {
    if (_array.size() == 0) {
      return std::set<T>();
    }
    std::set<T> objects;
    for (size_t i = 0; i < _array.size(); i++) {
      objects.insert(_array[i]);
    }
    return objects;
  }
};

/**
  * @brief Wrapper for char type
  *
  * gits::Cchar class is a wrapper for char
  * type value.
  */
class Cchar : public CArgument {
  char _value;

public:
  typedef CArgumentSizedArray<char, Cchar> CSArray;
  Cchar();
  Cchar(char);
  static const unsigned LENGTH = sizeof(char);
  static const char* NAME;

  virtual const char* Name() const {
    return NAME;
  }
  virtual unsigned Length() const {
    return LENGTH;
  }
  char Value() {
    return _value;
  }
  char Original() {
    return _value;
  }
  char operator*() {
    return _value;
  }

  virtual void Read(CBinIStream& stream);
  virtual void Write(CBinOStream& stream) const;
  virtual void Write(CCodeOStream& stream) const;
};

template <typename T>
class CByteStringArray : public CArgument {
  static_assert(sizeof(T) == 1, "CByteStringArray must use a type whose size is 1 byte");

  std::vector<Cchar::CSArray*> _cStringTable;
  std::vector<const T*> _constCharArray;
  std::vector<size_t> _lengthsArray;

public:
  CByteStringArray();
  CByteStringArray(const CByteStringArray& other) = delete;
  CByteStringArray& operator=(const CByteStringArray& other) = delete;
  ~CByteStringArray();
  CByteStringArray(uint32_t, const T**);
  CByteStringArray(uint32_t, T**);
  CByteStringArray(uint32_t, const void**);
  CByteStringArray(uint32_t, const T**, const size_t*);
  static const char* NAME;

  struct PtrConverter {
  private:
    const T** _ptr;

  public:
    explicit PtrConverter(const T** ptr) : _ptr(ptr) {}
    operator const T**() const {
      return _ptr;
    }
    operator const void**() const {
      return reinterpret_cast<const void**>(_ptr);
    }
  };

  virtual const char* Name() const {
    return NAME;
  }
  const T** Original();
  PtrConverter operator*() {
    return PtrConverter(Original());
  }
  virtual void Read(CBinIStream& stream);
  virtual void Write(CBinOStream& stream) const;
  virtual void Write(CCodeOStream& stream) const;
  virtual void Declare(CCodeOStream& stream) const;
  virtual bool DeclarationNeeded() const {
    return true;
  }

  size_t Count() const {
    return _cStringTable.size();
  }
  const size_t* Lengths() const {
    return _lengthsArray.data();
  }
  virtual std::set<uint64_t> GetMappedPointers() {
    return std::set<uint64_t>();
  }
};

typedef CByteStringArray<char> CStringArray;
typedef CByteStringArray<unsigned char> CBufferArray;

//************************** CStructArray ***********************************
template <class T, class WRAP_T, class T_GET_MAPPED_POINTERS = uint64_t>
class CStructArray : public CArgument {
public:
  typedef T TKey;
  typedef WRAP_T TKeyArg;

private:
  std::vector<std::shared_ptr<TKeyArg>> _cargs;
  std::vector<TKey> _data;
  std::vector<TKey> _originalData;

public:
  CStructArray() {}
  CStructArray(const TKey* dictionary) {
    if (dictionary == NULL) {
      return;
    }

    _cargs.resize(1);
    _cargs[0] = std::make_shared<TKeyArg>(&dictionary[0]);
  }
  CStructArray(int size, const TKey* dictionary) {
    if ((size == 0) || (dictionary == NULL)) {
      return;
    }

    _cargs.resize(size);
    for (int i = 0; i < size; i++) {
      _cargs[i] = std::make_shared<TKeyArg>(&dictionary[i]);
    }
  }
  template <class WRAP_T2>
  CStructArray(int size, const TKey* dictionary, const WRAP_T2 arg3) {
    if ((size == 0) || (dictionary == NULL)) {
      return;
    }

    _cargs.resize(size);
    for (int i = 0; i < size; i++) {
      _cargs[i] = std::make_shared<TKeyArg>(&dictionary[i], arg3);
    }
  }
  template <class WRAP_T2, class WRAP_T3>
  CStructArray(int size, const TKey* dictionary, const WRAP_T2* arg3, const WRAP_T3 arg4) {
    if ((size == 0) || (dictionary == NULL)) {
      return;
    }

    _cargs.resize(size);
    for (int i = 0; i < size; i++) {
      _cargs[i] = std::make_shared<TKeyArg>(&dictionary[i], arg3[i], arg4);
    }
  }
  std::vector<std::shared_ptr<TKeyArg>>& Vector() {
    return _cargs;
  }
  const std::vector<std::shared_ptr<TKeyArg>>& Vector() const {
    return _cargs;
  }

  TKey* Value() {
    if (_cargs.size() == 0) {
      return nullptr;
    }

    if (_data.size() == 0) { // Generate if not generated yet.
      _data.resize(_cargs.size());
      for (unsigned i = 0; i < _cargs.size(); i++) {
        _data[i] = **_cargs[i];
      }
    }

    return _data.data();
  }
  //const TKey* operator*() {
  //  return Value();
  //}
  TKey* operator*() {
    return Value();
  }
  TKey* Original() {
    if (_cargs.size() == 0) {
      return nullptr;
    }

    if (_originalData.size() == 0) { // Generate if not generated yet.
      _originalData.resize(_cargs.size());
      for (unsigned i = 0; i < _cargs.size(); i++) {
        _originalData[i] = _cargs[i]->Original();
      }
    }

    return _originalData.data();
  }

  const char* Name() const {
    // Not every CArgument has ::NAME, so add it manually if needed.
    // We can't just call Name() on the first argument of the vector because it may be empty.
    return TKeyArg::NAME;
  }
  virtual void Write(CBinOStream& stream) const {
    write_name_to_stream(stream, (unsigned)_cargs.size());
    for (auto& arg : _cargs) {
      arg->Write(stream);
    }
  }
  virtual void Read(CBinIStream& stream) {
    unsigned dictSize = 0;
    read_name_from_stream(stream, dictSize);
    if (dictSize <= _cargs.max_size()) {
      _cargs.resize(dictSize);
      for (unsigned i = 0; i < dictSize; i++) {
        std::shared_ptr<TKeyArg> keyArgPtr(new TKeyArg());
        keyArgPtr->Read(stream);
        _cargs[i] = std::move(keyArgPtr);
      }
    } else {
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
  }
  virtual void Write(CCodeOStream& stream) const {
    stream << getVarName("arr_", this);
  }
  virtual bool DeclarationNeeded() const {
    return true;
  }
  virtual void Declare(CCodeOStream& stream) const {
    const size_t length = _cargs.size();

    stream << "\n"; // For prettier printing of nested arrays.

    for (const auto& ptr : _cargs) {
      if (ptr->DeclarationNeeded()) {
        ptr->Declare(stream);
      }
    }

    stream.Indent() << Name();
    if (length == 0) {
      stream << "* " << getVarName("arr_", this) << " = nullptr;\n";
    } else {
      stream << " " << getVarName("arr_", this) << "[] = { ";
      size_t idx = 0;
      for (const auto& ptr : _cargs) {
        stream << *ptr;
        if (idx < length - 1) {
          stream << ", ";
        }
        idx++;
      }
      stream << " };\n";
    }
  }

  // Declares smaller parts of array, defined by start and end arguments.
  // Needed for dividing excessive chunks of code during writing to CCode.
  void Declare(CCodeOStream& stream, size_t start, size_t end) const {
    stream << "\n"; // For prettier printing of nested arrays.

    for (size_t i = start; i < end; i++) {
      if (_cargs[i]->DeclarationNeeded()) {
        _cargs[i]->Declare(stream);
      }
    }

    stream.Indent() << Name();
    if (start == end) {
      stream << "* " << getVarName("arr_", this) << " = nullptr;\n";
    } else {
      stream << " " << getVarName("arr_", this) << "[] = { ";
      for (size_t i = start; i < end; i++) {
        stream << *_cargs[i];
        if (i < end - 1) {
          stream << ", ";
        }
      }
      stream << " };\n";
    }
  }

  virtual bool PostActionNeeded() const {
    for (const auto& ptr : _cargs) {
      if (ptr->PostActionNeeded()) {
        return true;
      }
    }
    return false;
  }

  virtual void PostAction(CCodeOStream& stream) const {
    for (const auto& ptr : _cargs) {
      ptr->PostAction(stream);
    }
  }

  virtual std::set<T_GET_MAPPED_POINTERS> GetMappedPointers() {
    std::set<T_GET_MAPPED_POINTERS> returnMap;
    for (unsigned idx = 0; idx < _cargs.size(); idx++) {
      for (T_GET_MAPPED_POINTERS obj : _cargs[idx]->GetMappedPointers()) {
        returnMap.insert(obj);
      }
    }
    return returnMap;
  }
};

//************************** CStructArrayOfArrays ***********************************
template <class T, class WRAP_T, class T_GET_MAPPED_POINTERS = uint64_t>
class CStructArrayOfArrays : public CArgument {
public:
  typedef T TKey;
  typedef WRAP_T TKeyArg;

private:
  std::vector<std::vector<std::shared_ptr<TKeyArg>>> _cargs; // wrapped data
  std::vector<std::vector<TKey>> _dataStorage;               // array of arrays of elements
  std::vector<TKey*> _data;                            // array of pointers to individual elements
  std::vector<std::vector<TKey>> _originalDataStorage; // array of arrays of original values
  std::vector<TKey*> _originalData;                    // array of pointers to original elements
public:
  CStructArrayOfArrays() {}

  CStructArrayOfArrays(int size, const TKey* const* dictionary) {
    if ((size == 0) || (dictionary == nullptr)) {
      return;
    }

    _cargs.resize(size);
    for (int i = 0; i < size; i++) {
      _cargs[i].resize(1);
      _cargs[i][0] = std::make_shared<TKeyArg>(dictionary[i]);
    }
  }

  template <class WRAP_T2>
  CStructArrayOfArrays(int size, const TKey* const* dictionary, const WRAP_T2 arg3) {
    if ((size == 0) || (dictionary == nullptr)) {
      return;
    }

    _cargs.resize(size);
    for (int i = 0; i < size; i++) {
      _cargs[i].resize(1);
      _cargs[i][0] = std::make_shared<TKeyArg>(dictionary[i], arg3);
    }
  }

  template <class WRAP_T2, class WRAP_T3>
  CStructArrayOfArrays(int size,
                       const TKey* const* dictionary,
                       const WRAP_T2* arg3,
                       const WRAP_T3 arg4) {
    if ((size == 0) || (dictionary == nullptr)) {
      return;
    }

    _cargs.resize(size);
    for (int i = 0; i < size; i++) {
      _cargs[i].resize(1);
      _cargs[i][0] = std::make_shared<TKeyArg>(dictionary[i], arg3[i], arg4);
    }
  }

  CStructArrayOfArrays(std::vector<uint32_t> const& sizes, const TKey* const* dictionary) {
    if ((sizes.size() == 0) || (dictionary == nullptr)) {
      return;
    }

    _cargs.resize(sizes.size());
    for (size_t i = 0; i < sizes.size(); i++) {
      _cargs[i].resize(sizes[i]);
      for (uint32_t j = 0; j < sizes[i]; j++) {
        _cargs[i][j] = std::make_shared<TKeyArg>(&dictionary[i][j]);
      }
    }
  }

  template <class WRAP_T2>
  CStructArrayOfArrays(std::vector<uint32_t> const& sizes,
                       const TKey* const* dictionary,
                       const WRAP_T2 arg3) {
    if ((sizes.size() == 0) || (dictionary == nullptr)) {
      return;
    }

    _cargs.resize(sizes.size());
    for (size_t i = 0; i < sizes.size(); i++) {
      _cargs[i].resize(sizes[i]);
      for (uint32_t j = 0; j < sizes[i]; j++) {
        _cargs[i][j] = std::make_shared<TKeyArg>(&dictionary[i][j], arg3);
      }
    }
  }

  std::vector<std::shared_ptr<TKeyArg>>& Vector() {
    return _cargs;
  }
  const std::vector<std::shared_ptr<TKeyArg>>& Vector() const {
    return _cargs;
  }

  TKey* const* Value() {
    if (_cargs.size() == 0) {
      return nullptr;
    }

    if (_data.size() == 0) { // Generate if not generated yet.
      _dataStorage.resize(_cargs.size());
      _data.resize(_cargs.size());
      for (unsigned i = 0; i < _cargs.size(); i++) {
        _dataStorage[i].resize(_cargs[i].size());

        for (unsigned j = 0; j < _cargs[i].size(); j++) {
          _dataStorage[i][j] = **_cargs[i][j];
        }
        _data[i] = _dataStorage[i].data();
      }
    }

    return _data.data();
  }
  TKey* const* operator*() {
    return Value();
  }
  TKey* const* Original() {
    if (_cargs.size() == 0) {
      return nullptr;
    }

    if (_originalData.size() == 0) { // Generate if not generated yet.
      _originalDataStorage.resize(_cargs.size());
      _originalData.resize(_cargs.size());
      for (unsigned i = 0; i < _cargs.size(); i++) {
        _originalDataStorage[i].resize(_cargs[i].size());

        for (unsigned j = 0; j < _cargs[i].size(); j++) {
          _originalDataStorage[i][j] = _cargs[i][j]->Original();
        }
        _originalData[i] = _originalDataStorage[i].data();
      }
    }

    return _originalData.data();
  }

  const char* Name() const {
    // Not every CArgument has ::NAME, so add it manually if needed.
    // We can't just call Name() on the first argument of the vector because it may be empty.
    return TKeyArg::NAME;
  }

  virtual void Write(CBinOStream& stream) const {
    write_name_to_stream(stream, (unsigned)_cargs.size());
    for (auto& outer : _cargs) {
      write_name_to_stream(stream, (unsigned)outer.size());
      for (auto& inner : outer) {
        inner->Write(stream);
      }
    }
  }

  virtual void Read(CBinIStream& stream) {
    unsigned outerDictSize;
    read_name_from_stream(stream, outerDictSize);

    if (outerDictSize <= _cargs.max_size()) {
      _cargs.resize(outerDictSize);

      for (unsigned i = 0; i < outerDictSize; i++) {
        unsigned innerDictSize;
        read_name_from_stream(stream, innerDictSize);
        if (innerDictSize <= _cargs[i].max_size()) {
          _cargs[i].resize(innerDictSize);

          for (unsigned j = 0; j < innerDictSize; j++) {
            std::shared_ptr<TKeyArg> keyArgPtr(new TKeyArg());
            keyArgPtr->Read(stream);
            _cargs[i][j] = std::move(keyArgPtr);
          }
        } else {
          throw std::runtime_error(EXCEPTION_MESSAGE);
        }
      }
    } else {
      throw std::runtime_error(EXCEPTION_MESSAGE);
    }
  }

  virtual void Write(CCodeOStream& stream) const {
    stream << getVarName("arr_", this);
  }
  virtual bool DeclarationNeeded() const {
    return true;
  }

  void Declare(CCodeOStream& stream) const {
    TODO("Finish declaration for CCode - important for ray tracing")
  }

  virtual bool PostActionNeeded() const {
    for (const auto& outer : _cargs) {
      for (const auto& inner : outer) {
        if (inner->PostActionNeeded()) {
          return true;
        }
      }
    }
    return false;
  }

  virtual void PostAction(CCodeOStream& stream) const {
    for (const auto& outer : _cargs) {
      for (const auto& inner : outer) {
        inner->PostAction(stream);
      }
    }
  }

  virtual std::set<T_GET_MAPPED_POINTERS> GetMappedPointers() {
    std::set<T_GET_MAPPED_POINTERS> returnMap;
    for (unsigned outer = 0; outer < _cargs.size(); outer++) {
      for (unsigned inner = 0; inner < _cargs[outer].size(); inner++) {
        for (T_GET_MAPPED_POINTERS obj : _cargs[outer][inner]->GetMappedPointers()) {
          returnMap.insert(obj);
        }
      }
    }
    return returnMap;
  }
};

/**
  * @brief Wrapper for int type
  *
  * gits::Cint class is a wrapper for int
  * type value.
  */
class Cint : public CArgument {
  int _value;

public:
  typedef CArgumentSizedArray<int, Cint> CSArray;
  Cint() : _value(0) {}
  Cint(int value) : _value(value) {}
  static const unsigned LENGTH = sizeof(int);
  static const char* NAME;

  virtual const char* Name() const {
    return NAME;
  }
  virtual unsigned Length() const {
    return LENGTH;
  }
  int& Value() {
    return _value;
  }
  int& Original() {
    return _value;
  }
  int& operator*() {
    return _value;
  }

  virtual void Read(CBinIStream& stream);
  virtual void Write(CBinOStream& stream) const;
  virtual void Write(CCodeOStream& stream) const;
};

//************************** Cuint64_t ***********************************
class Cuint64_t : public CArgument {
  uint64_t _uint64;

public:
  static const char* NAME;
  typedef CArgumentSizedArray<uint64_t, Cuint64_t> CSArray;
  Cuint64_t() : _uint64(0) {}
  Cuint64_t(uint64_t uint) : _uint64(uint) {}
  uint64_t operator*() const {
    return _uint64;
  }
  uint64_t Original() const {
    return _uint64;
  }
  const char* Name() const {
    return NAME;
  }
  virtual void Write(CBinOStream& stream) const {
    write_name_to_stream(stream, _uint64);
  }
  virtual void Read(CBinIStream& stream) {
    read_name_from_stream(stream, _uint64);
  }
  virtual void Write(CCodeOStream& stream) const {
    stream << _uint64;
  }
};

//************************** Cint64_t ***********************************
class Cint64_t : public CArgument {
  int64_t _value;

public:
  static const char* NAME;
  typedef CArgumentSizedArray<int64_t, Cint64_t> CSArray;
  Cint64_t() : _value(0) {}
  Cint64_t(int64_t value) : _value(value) {}
  int64_t operator*() const {
    return _value;
  }
  int64_t Original() const {
    return _value;
  }
  const char* Name() const {
    return NAME;
  }
  virtual void Write(CBinOStream& stream) const {
    write_name_to_stream(stream, _value);
  }
  virtual void Read(CBinIStream& stream) {
    read_name_from_stream(stream, _value);
  }
  virtual void Write(CCodeOStream& stream) const {
    stream << _value;
  }
};

//************************** Cdouble ***********************************
class Cdouble : public CArgument {
  double _double;

public:
  static const char* NAME;
  typedef CArgumentSizedArray<double, Cdouble> CSArray;
  Cdouble() : _double(0.0) {}
  Cdouble(double uint) : _double(uint) {}
  double operator*() const {
    return _double;
  }
  double Original() const {
    return _double;
  }
  const char* Name() const {
    return "double";
  }
  virtual void Write(CBinOStream& stream) const {
    write_name_to_stream(stream, _double);
  }
  virtual void Read(CBinIStream& stream) {
    read_name_from_stream(stream, _double);
  }
  virtual void Write(CCodeOStream& stream) const {
    stream << _double;
  }
};

//************************** Cfloat ***********************************
class Cfloat : public CArgument {
  float _float;

public:
  static const char* NAME;
  typedef CArgumentSizedArray<float, Cfloat> CSArray;
  Cfloat() : _float(0.0f) {}
  Cfloat(float value) : _float(value) {}
  float operator*() const {
    return _float;
  }
  float Original() const {
    return _float;
  }
  const char* Name() const {
    return "float";
  }
  virtual void Write(CBinOStream& stream) const {
    write_name_to_stream(stream, _float);
  }
  virtual void Read(CBinIStream& stream) {
    read_name_from_stream(stream, _float);
  }
  virtual void Write(CCodeOStream& stream) const {
    if (std::isinf(_float)) {
      stream << "INFINITY";
    } else {
      stream << _float;
    }
  }
};

//************************** Cuint8_t ***********************************
class Cuint8_t : public CArgument {
protected:
  uint8_t _uint8;

public:
  static const char* NAME;
  typedef CArgumentSizedArray<uint8_t, Cuint8_t> CSArray;

  Cuint8_t() : _uint8(0u) {}
  Cuint8_t(uint8_t uint) : _uint8(uint) {}
  uint8_t operator*() const {
    return _uint8;
  }
  uint8_t Original() const {
    return _uint8;
  }
  const char* Name() const {
    return NAME;
  }
  virtual void Write(CBinOStream& stream) const {
    write_name_to_stream(stream, _uint8);
  }
  virtual void Read(CBinIStream& stream) {
    read_name_from_stream(stream, _uint8);
  }
  virtual void Write(CCodeOStream& stream) const {
    stream << (uint32_t)_uint8; // Cast so it gets written as text, not raw bytes.
  }
};

//************************** Cint32_t ***********************************
class Cint8_t : public CArgument {
protected:
  int8_t _value;

public:
  static const char* NAME;
  typedef CArgumentSizedArray<int8_t, Cint8_t> CSArray;

  Cint8_t() : _value(0) {}
  Cint8_t(int8_t value) : _value(value) {}
  int8_t operator*() const {
    return _value;
  }
  int8_t Original() const {
    return _value;
  }
  const char* Name() const {
    return NAME;
  }
  virtual void Write(CBinOStream& stream) const {
    write_name_to_stream(stream, _value);
  }
  virtual void Read(CBinIStream& stream) {
    read_name_from_stream(stream, _value);
  }
  virtual void Write(CCodeOStream& stream) const {
    stream << (int32_t)_value; // Cast so it gets written as text, not raw bytes.
  }
};

//************************** Cuint16_t ***********************************
class Cuint16_t : public CArgument {
protected:
  uint16_t _uint16;

public:
  static const char* NAME;
  typedef CArgumentSizedArray<uint16_t, Cuint16_t> CSArray;

  Cuint16_t() : _uint16(0u) {}
  Cuint16_t(uint16_t uint) : _uint16(uint) {}
  uint16_t operator*() const {
    return _uint16;
  }
  uint16_t Original() const {
    return _uint16;
  }
  const char* Name() const {
    return NAME;
  }
  virtual void Write(CBinOStream& stream) const {
    write_name_to_stream(stream, _uint16);
  }
  virtual void Read(CBinIStream& stream) {
    read_name_from_stream(stream, _uint16);
  }
  virtual void Write(CCodeOStream& stream) const {
    stream << _uint16;
  }
};

//************************** Cint16_t ***********************************
class Cint16_t : public CArgument {
protected:
  int16_t _int;

public:
  static const char* NAME;
  typedef CArgumentSizedArray<int16_t, Cint16_t> CSArray;
  Cint16_t() : _int(0) {}
  Cint16_t(int16_t int16) : _int(int16) {}
  int16_t operator*() const {
    return _int;
  }
  int16_t Original() const {
    return _int;
  }
  const char* Name() const {
    return NAME;
  }
  virtual void Write(CBinOStream& stream) const {
    write_name_to_stream(stream, _int);
  }
  virtual void Read(CBinIStream& stream) {
    read_name_from_stream(stream, _int);
  }
  virtual void Write(CCodeOStream& stream) const {
    stream << _int;
  }
};

//************************** Cuint32_t ***********************************
class Cuint32_t : public CArgument {
protected:
  uint32_t _uint32;

public:
  static const char* NAME;
  typedef CArgumentSizedArray<uint32_t, Cuint32_t> CSArray;

  Cuint32_t() : _uint32(0u) {}
  Cuint32_t(uint32_t uint) : _uint32(uint) {}
  uint32_t operator*() const {
    return _uint32;
  }
  uint32_t Original() const {
    return _uint32;
  }
  const char* Name() const {
    return NAME;
  }
  virtual void Write(CBinOStream& stream) const {
    write_name_to_stream(stream, _uint32);
  }
  virtual void Read(CBinIStream& stream) {
    read_name_from_stream(stream, _uint32);
  }
  virtual void Write(CCodeOStream& stream) const {
    stream << _uint32;
  }
};

//************************** Cint32_t ***********************************
class Cint32_t : public CArgument {
protected:
  int32_t _int;

public:
  static const char* NAME;
  typedef CArgumentSizedArray<int32_t, Cint32_t> CSArray;
  Cint32_t() : _int(0) {}
  Cint32_t(int32_t int32) : _int(int32) {}
  int32_t operator*() const {
    return _int;
  }
  int32_t Original() const {
    return _int;
  }
  const char* Name() const {
    return NAME;
  }
  virtual void Write(CBinOStream& stream) const {
    write_name_to_stream(stream, _int);
  }
  virtual void Read(CBinIStream& stream) {
    read_name_from_stream(stream, _int);
  }
  virtual void Write(CCodeOStream& stream) const {
    stream << _int;
  }
};

//************************** Csize_t ***********************************
class Csize_t : public CArgument {
protected:
  size_t _size;

public:
  static const char* NAME;
  typedef CArgumentSizedArray<size_t, Csize_t> CSArray;

  Csize_t() : _size(0u) {}
  Csize_t(size_t size) : _size(size) {}
  size_t operator*() const {
    return _size;
  }
  size_t Original() const {
    return _size;
  }
  const char* Name() const {
    return NAME;
  }
  virtual void Write(CBinOStream& stream) const {
    write_size_t_to_stream(stream, _size);
  }
  virtual void Read(CBinIStream& stream) {
    read_size_t_from_stream(stream, _size);
  }
  virtual void Write(CCodeOStream& stream) const {
    stream << _size;
  }
};

//**************************  CBOOL ***********************************
class Cbool : public CArgument {
  bool _BOOL;

public:
  Cbool() : _BOOL(false) {}
  Cbool(bool bool_) : _BOOL(bool_) {}
  bool& Value() {
    return _BOOL;
  }
  bool operator*() const {
    return (bool)_BOOL;
  }
  void Assign(bool bool_) {
    _BOOL = bool_;
  }
  const char* Name() const {
    return "bool";
  }
  virtual void Write(CBinOStream& stream) const {
    write_name_to_stream(stream, _BOOL);
  }
  virtual void Read(CBinIStream& stream) {
    read_name_from_stream(stream, _BOOL);
  }
  virtual void Write(CCodeOStream& stream) const {
    stream << (_BOOL ? "true" : "false");
  }
};

// **************************  CMappedHandle  **************************
// Maps external memory handles used in recorder to player handles.
class CMappedHandle : public CArgument {
  static constexpr uint32_t currentVersion_ = 0;
  Cuint32_t version_;
  void* handle_;

public:
  typedef CArgumentMappedSizedArray<void*, CMappedHandle, gits::ADD_MAPPING> CSMapArray;
  typedef CArgumentMappedSizedArray<void*, CMappedHandle, gits::NO_ACTION> CSArray;

  CMappedHandle();
  CMappedHandle(void* arg);

  static const char* NAME;

  virtual const char* Name() const override;
  static const char* TypeNameStr();
  static const char* WrapTypeNameStr();

  virtual void Write(CBinOStream& stream) const override;
  virtual void Read(CBinIStream& stream) override;
  virtual void Write([[maybe_unused]] CCodeOStream& stream) const override;

  void* Original() const;
  void* Value() const;
  void* operator*() const;

  std::set<uint64_t> GetMappedPointers();

  // Mapping methods:
  // TODO: the mapping logic should be in one class; every other mapping class should inherit it
  // instead of copy-pasting it. Perhaps some classes could even be typedefs.
  static void AddMapping(void* key, void* value);
  static void RemoveMapping(void* key);
  static void* GetMapping(void* key);
  static bool CheckMapping(void* key);
  // NOTE: If you are getting collisions of different handle types, separate the types by making
  // this class a template parametrized by type tags.

private:
  typedef std::unordered_map<void*, void*> handle_map_t;
  static handle_map_t& get_map();
};

} // namespace gits

template <class T, class T_WRAP>
gits::CArgumentSizedArrayBase<T, T_WRAP>::CArgumentSizedArrayBase(size_t num) {
  if (num == 0) {
    return;
  }

  _array.resize(num);
}

template <class T, class T_WRAP>
gits::CArgumentSizedArrayBase<T, T_WRAP>::CArgumentSizedArrayBase(size_t num, const T* init) {
  if (num == 0) {
    return;
  }
  if (init == nullptr) {
    return;
  }

  _array.resize(num);
  for (size_t i = 0; i < num; i++) {
    _array[i] = init[i];
  }
}

template <class T, class T_WRAP>
gits::CArgumentSizedArrayBase<T, T_WRAP>::CArgumentSizedArrayBase(const std::vector<T>& array)
    : _array(array) {}

template <class T, class T_WRAP>
gits::CArgumentSizedArrayBase<T, T_WRAP>::CArgumentSizedArrayBase(const T* init,
                                                                  T terminator,
                                                                  int term_pos) {
  if (init == nullptr) {
    return;
  }

  for (size_t i = 0;; i++) {
    _array.push_back(init[i]);
    if ((i % term_pos == 0) && (_array.back() == terminator)) {
      //we just pushed the terminator, exit
      break;
    }
  }
}

template <class T, class T_WRAP>
void gits::CArgumentSizedArrayBase<T, T_WRAP>::Write(CBinOStream& stream) const {
  uint32_t size = ensure_unsigned32bit_representible<size_t>(_array.size());
  if (!Config::Get().recorder.extras.utilities.nullIO) {
    stream.write((char*)&size, sizeof(size));
  }

  if (size != 0) {
    for (unsigned idx = 0; idx < size; idx++) {
      CGLtype wrapper_(_array[idx]);
      stream << wrapper_;
    }
  }
}

template <class T, class T_WRAP>
void gits::CArgumentSizedArrayBase<T, T_WRAP>::Read(CBinIStream& stream) {
  uint32_t size = 0U;
  const auto ret = stream.read((char*)&size, sizeof(size));
  if (size <= UINT32_MAX && ret) {
    if (size != 0U) {
      _array.resize(size);
      static bool isfloat = std::is_floating_point<T>::value;
      if (!isfloat) {
        for (auto idx = 0U; idx < size; idx++) {
          CGLtype wrapper_;
          stream >> wrapper_;
          _array[idx] = wrapper_.Original();
        }
      } else {
        // floats never need to adjust their type
        stream.read((char*)&_array[0], size * sizeof(T));
      }
    }
  }
}

template <class T, class T_WRAP>
void gits::CArgumentSizedArrayBase<T, T_WRAP>::Write(CCodeOStream& stream) const {
  stream << stream.VariableName(ScopeKey());
}

template <class T, class T_WRAP>
void gits::CArgumentSizedArrayBase<T, T_WRAP>::Declare(CCodeOStream& stream) const {
  const size_t arr_len = _array.size();

  stream.Indent() << Name();

  if (arr_len == 0) {
    stream << "*";
  }
  stream << " " << stream.VariableName(ScopeKey());
  if (arr_len == 0) {
    stream << " = nullptr;\n";
  } else {
    stream << "[] = { ";
    size_t idx = 0;
    for (const auto& elem : _array) {
      CGLtype wrapper_(elem);
      stream << wrapper_;
      if (idx < arr_len - 1) {
        stream << ", ";
      }
      idx++;
    }
    stream << " };\n";
  }
}

template <class T_WRAP>
void gits::CArgumentSizedArray<char, T_WRAP>::Declare(CCodeOStream& stream) const {
  VariableNameRegister(stream, false);
  stream.Indent() << Name() << " " << stream.VariableName(ScopeKey()) << "[] = ";

  if ((int)_sizedArray.Vector().size() > 0 && _sizedArray.Vector().back() == '\0') {
    stream << "\"" << (const char*)&_sizedArray.Vector()[0] << "\";\n";
  } else {
    // declare an array
    stream << "{ ";

    // initiate all elements in an array
    if ((int)_sizedArray.Vector().size() == 0) {
      stream << "0";
    } else {
      for (auto iter = _sizedArray.Vector().begin(); iter != _sizedArray.Vector().end(); iter++) {
        CGLtype wrapper_(*iter);
        stream << wrapper_;
        if (iter < _sizedArray.Vector().end() - 1) {
          stream << ", ";
        }
      }
    }
    stream << " };\n";
  }
}

template <class T, class T_WRAP, gits::MappedArrayAction T_ACTION>
gits::CArgumentMappedSizedArray<T, T_WRAP, T_ACTION>::CArgumentMappedSizedArray(size_t num) {
  _array.resize(num);
}

template <class T, class T_WRAP, gits::MappedArrayAction T_ACTION>
gits::CArgumentMappedSizedArray<T, T_WRAP, T_ACTION>::CArgumentMappedSizedArray(size_t num,
                                                                                const T* array) {
  if (array == 0) {
    return;
  }

  _array.resize(num);
  for (size_t i = 0; i < num; i++) {
    _array[i] = array[i];
  }
}

template <class T, class T_WRAP, gits::MappedArrayAction T_ACTION>
gits::CArgumentMappedSizedArray<T, T_WRAP, T_ACTION>::CArgumentMappedSizedArray(
    const std::vector<T>& array)
    : _array(array) {}
// expect pointer to 'terminator' ended array, consider values to be terminator
// only when it has specific value and is at position 'i == 0 % term_pos'
template <class T, class T_WRAP, gits::MappedArrayAction T_ACTION>
gits::CArgumentMappedSizedArray<T, T_WRAP, T_ACTION>::CArgumentMappedSizedArray(const T* array,
                                                                                T terminator,
                                                                                int term_pos) {
  if (array == 0) {
    return;
  }

  for (size_t i = 0;; i++) {
    _array.push_back(array[i]);
    if ((i % term_pos == 0) && (_array.back() == terminator)) {
      // we just pushed the terminator, exit
      break;
    }
  }
}

template <class T, class T_WRAP, gits::MappedArrayAction T_ACTION>
gits::CArgumentMappedSizedArray<T, T_WRAP, T_ACTION>::CArgumentMappedSizedArray(const T* array) {
  if (array == 0) {
    return;
  }

  _array.resize(1);
  _array[0] = array[0];
}

template <class T, class T_WRAP, gits::MappedArrayAction T_ACTION>
void gits::CArgumentMappedSizedArray<T, T_WRAP, T_ACTION>::Write(CBinOStream& stream) const {
  uint32_t size = ensure_unsigned32bit_representible<size_t>(_array.size());
  if (!Config::Get().recorder.extras.utilities.nullIO) {
    stream.write((char*)&size, sizeof(size));
  }

  if (size != 0) {
    for (unsigned idx = 0; idx < size; idx++) {
      CGLtype wrapper_(_array[idx]);
      stream << wrapper_;
    }
  }
}

template <class T, class T_WRAP, gits::MappedArrayAction T_ACTION>
void gits::CArgumentMappedSizedArray<T, T_WRAP, T_ACTION>::Read(CBinIStream& stream) {
  uint32_t size = 0U;
  const auto ret = stream.read((char*)&size, sizeof(size));
  if (size > 0 && size <= UINT32_MAX && ret) {
    _array.resize(size);
    for (unsigned idx = 0; idx < size; idx++) {
      CGLtype wrapper_;
      stream >> wrapper_;
      _array[idx] = wrapper_.Original();
    }
  }
}

template <class T, class T_WRAP, gits::MappedArrayAction T_ACTION>
void gits::CArgumentMappedSizedArray<T, T_WRAP, T_ACTION>::Write(CCodeOStream& stream) const {
  stream << stream.VariableName(ScopeKey());
}

template <class T, class T_WRAP, gits::MappedArrayAction T_ACTION>
void gits::CArgumentMappedSizedArray<T, T_WRAP, T_ACTION>::WriteArray(CCodeOStream& stream,
                                                                      std::string name,
                                                                      ValuesType valtype) const {

  if (this->Action() == NO_ACTION) {
    stream.Indent() << Name();
  } else {
    stream.Indent() << CGLtype::TypeNameStr();
  }

  if ((int)_array.size() == 0) {
    stream << "*";
  }
  stream << " " << name;
  if ((int)_array.size() != 0) {
    stream << "[]";
  }
  // Initialize all elements in an array.
  if ((int)_array.size() == 0) {
    stream << " = nullptr;\n";
  } else {
    // Declare an array.
    stream << " = { ";
    for (auto iter = _array.begin(); iter != _array.end(); iter++) {
      if (valtype == MAPPEDS) {
        CGLtype wrapper_(*iter);
        stream << wrapper_;
      } else if (valtype == ORIGS) {
        stream << "(" << CGLtype::TypeNameStr() << ") " << hex(*iter);
      } else if (valtype == NULLS) {
        stream << 0;
      } else {
        throw std::runtime_error(EXCEPTION_MESSAGE);
      }
      if (iter < _array.end() - 1) {
        stream << ", ";
      }
    }
    stream << " };\n";
  }
}

template <class T, class T_WRAP, gits::MappedArrayAction T_ACTION>
void gits::CArgumentMappedSizedArray<T, T_WRAP, T_ACTION>::WritePartArray(
    CCodeOStream& stream, std::string name, ValuesType valtype, size_t start, size_t end) const {

  if (this->Action() == NO_ACTION) {
    stream.Indent() << Name();
  } else {
    stream.Indent() << CGLtype::TypeNameStr();
  }

  if ((int)_array.size() == 0) {
    stream << "*";
  }
  stream << " " << name;
  if ((int)_array.size() != 0) {
    stream << "[]";
  }
  // Initialize all elements in an array.
  if ((int)_array.size() == 0) {
    stream << " = nullptr;\n";
  } else {
    // Declare an array.
    stream << " = { ";
    for (auto iter = _array.begin() + start; iter != _array.begin() + end; iter++) {
      if (valtype == MAPPEDS) {
        CGLtype wrapper_(*iter);
        stream << wrapper_;
      } else if (valtype == ORIGS) {
        stream << "(" << CGLtype::TypeNameStr() << ") " << hex(*iter);
      } else if (valtype == NULLS) {
        stream << 0;
      } else {
        throw std::runtime_error(EXCEPTION_MESSAGE);
      }
      if (iter < _array.end() - 1) {
        stream << ", ";
      }
    }
    stream << " };\n";
  }
}

template <class T, class T_WRAP, gits::MappedArrayAction T_ACTION>
void gits::CArgumentMappedSizedArray<T, T_WRAP, T_ACTION>::Declare(CCodeOStream& stream) const {
  if (this->Action() == ADD_MAPPING) {
    WriteArray(stream, stream.VariableName(ScopeKey()), ORIGS);
  } else {
    WriteArray(stream, stream.VariableName(ScopeKey()), MAPPEDS);
  }
}

template <class T, class T_WRAP, gits::MappedArrayAction T_ACTION>
void gits::CArgumentMappedSizedArray<T, T_WRAP, T_ACTION>::Declare(CCodeOStream& stream,
                                                                   size_t start,
                                                                   size_t end) const {
  if (this->Action() == ADD_MAPPING) {
    WritePartArray(stream, stream.VariableName(ScopeKey()), ORIGS, start, end);
  } else {
    WritePartArray(stream, stream.VariableName(ScopeKey()), MAPPEDS, start, end);
  }
}

template <class T, class T_WRAP, gits::MappedArrayAction T_ACTION>
void gits::CArgumentMappedSizedArray<T, T_WRAP, T_ACTION>::PostAction(CCodeOStream& stream) const {
  static bool mappingsAreFreeFunctions =
      gits::CGits::Instance().apis.Has3D() &&
      gits::CGits::Instance().apis.Iface3D().Api() == ApisIface::Vulkan;

  if (_array.size() == 0) {
    return;
  }

  if (this->Action() == ADD_MAPPING) {
    std::string origArrayName = stream.VariableName(ScopeKey()) + "_orig";
    WriteArray(stream, origArrayName, ORIGS);
    if (mappingsAreFreeFunctions) {
      stream.Indent() << "AddMapping(" << origArrayName << ", " << stream.VariableName(ScopeKey())
                      << ", " << _array.size() << ");\n";
    } else {
      stream.Indent() << CGLtype::WrapTypeNameStr() << "::AddMapping(" << origArrayName << ", "
                      << stream.VariableName(ScopeKey()) << ", " << _array.size() << ");\n";
    }
  } else if (this->Action() == REMOVE_MAPPING) {
    if (mappingsAreFreeFunctions) {
      stream.Indent() << "RemoveMapping(" << stream.VariableName(ScopeKey()) << ", "
                      << _array.size() << ");\n";
    } else {
      stream.Indent() << CGLtype::WrapTypeNameStr() << "::RemoveMapping("
                      << stream.VariableName(ScopeKey()) << ", " << _array.size() << ");\n";
    }
  }
}

template <class T, class T_WRAP, gits::MappedArrayAction T_ACTION>
void gits::CArgumentMappedSizedArray<T, T_WRAP, T_ACTION>::PostAction(CCodeOStream& stream,
                                                                      size_t start,
                                                                      size_t end) const {
  static bool mappingsAreFreeFunctions =
      gits::CGits::Instance().apis.Has3D() &&
      gits::CGits::Instance().apis.Iface3D().Api() == ApisIface::Vulkan;

  if (_array.size() == 0) {
    return;
  }

  if (this->Action() == ADD_MAPPING) {
    std::string origArrayName = stream.VariableName(ScopeKey()) + "_orig";
    WritePartArray(stream, origArrayName, ORIGS, start, end);
    if (mappingsAreFreeFunctions) {
      stream.Indent() << "AddMapping(" << origArrayName << ", " << stream.VariableName(ScopeKey())
                      << ", " << end - start << ");\n";
    } else {
      stream.Indent() << CGLtype::WrapTypeNameStr() << "::AddMapping(" << origArrayName << ", "
                      << stream.VariableName(ScopeKey()) << ", " << end - start << ");\n";
    }
  } else if (this->Action() == REMOVE_MAPPING) {
    if (mappingsAreFreeFunctions) {
      stream.Indent() << "RemoveMapping(" << stream.VariableName(ScopeKey()) << ", " << end - start
                      << ");\n";
    } else {
      stream.Indent() << CGLtype::WrapTypeNameStr() << "::RemoveMapping("
                      << stream.VariableName(ScopeKey()) << ", " << end - start << ");\n";
    }
  }
}

template <class T, int N, class T_WRAP>
void gits::CArgumentFixedArray<T, N, T_WRAP>::Read(CBinIStream& stream) {
  for (unsigned idx = 0; idx < N; idx++) {
    CGLtype wrapper_;
    stream >> wrapper_;
    _array[idx] = *wrapper_;
  }
}

template <class T, int N, class T_WRAP>
void gits::CArgumentFixedArray<T, N, T_WRAP>::Write(CCodeOStream& stream) const {
  stream << stream.VariableName(ScopeKey());
}

template <class T, int N, class T_WRAP>
void gits::CArgumentFixedArray<T, N, T_WRAP>::Write(CBinOStream& stream) const {
  for (int idx = 0; idx < Size(); idx++) {
    CGLtype wrapper_(_array[idx]);
    stream << wrapper_;
  }
}

template <class T, int N, class T_WRAP>
void gits::CArgumentFixedArray<T, N, T_WRAP>::Declare(CCodeOStream& stream,
                                                      const std::string& declString) const {
  // declare an array
  stream.Indent() << declString;
  if (Size() == 1) {
    stream << " = ";
    DeclareElement(0, stream);
  } else {
    stream << " = { ";
    stream.ScopeBegin();
    // initiate all elements in an array
    for (int idx = 0; idx < Size(); idx++) {
      DeclareElement(idx, stream);
      if (idx < Size() - 1) {
        stream << ", ";
      }
      if (!((idx + 1) % 16) && (idx != Size() - 1)) {
        stream << "\n";
        stream.Indent();
      }
    }
    stream.ScopeEnd();
    stream << " }";
  }
}

template <class T, int N, class T_WRAP>
void gits::CArgumentFixedArray<T, N, T_WRAP>::Declare(CCodeOStream& stream) const {
  stream.Indent() << Name() << " " << stream.VariableName(ScopeKey()) << "[] = ";
  // declare an array
  stream << "{ ";

  for (int i = 0; i < N; i++) {
    CGLtype wrapper_(_array[i]);
    stream << wrapper_;
    if (i < N - 1) {
      stream << ", ";
    }
  }
  stream << " };\n";
}
