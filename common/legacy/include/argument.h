// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
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
#include <string>

namespace gits {
class CBinOStream;
class CBinIStream;

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

  virtual void* GetPtrType() {
    throw ENotImplemented(EXCEPTION_MESSAGE);
  }

  static bool InitializedWithOriginal() {
    return false;
  }

  virtual std::string ToString() const {
    return "";
  }

  virtual uint64_t Size() const {
    if (CGits::Instance().GetApi3D() == ApisIface::ApisIface::OpenGL) {
      throw std::runtime_error(std::string("Size() not implemented for argument wrapper ") +
                               Name() + " used in OpenGL API.");
    } else if (CGits::Instance().GetApi3D() == ApisIface::ApisIface::Vulkan) {
      throw std::runtime_error(std::string("Size() not implemented for argument wrapper ") +
                               Name() + " used in Vulkan API.");
    }

    return 0;
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
  virtual uint64_t Size() const override;
};

class CArgumentFileText : public CArgument {
protected:
  std::string _fileName;
  std::string _text;

  void init(const char* fileName, const char* text, unsigned length);
  void LoadTextFromFile();

public:
  CArgumentFileText() {}
  CArgumentFileText(const std::string& fileName, const std::string& text);
  CArgumentFileText(const char* fileName, const char* text, unsigned length);
  virtual const char* Name() const {
    return _fileName.c_str();
  }
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);

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

  // Serialized as: filename length+bytes and text length+bytes
  virtual uint64_t Size() const override {
    // Two 32-bit length headers plus raw bytes
    return sizeof(uint32_t) + _fileName.size() + sizeof(uint32_t) + _text.size();
  }
};

class CBinaryResource : public CArgument {
public:
  struct PointerProxy {
    PointerProxy() : _ptr(0), _size(0) {}
    explicit PointerProxy(const void* ptr) : _ptr(ptr), _size(0) {}
    explicit PointerProxy(const void* ptr, size_t size) : _ptr(ptr), _size(size) {}
    template <class T>
    operator const T*() const {
      return (const T*)_ptr;
    }
    operator const void*() const {
      return _ptr;
    }
    size_t Size() const {
      return _size;
    }

  private:
    const void* _ptr;
    size_t _size;
  };

  explicit CBinaryResource();
  explicit CBinaryResource(hash_t hash);
  CBinaryResource(TResourceType type, const void* data, size_t size);

  void reset(TResourceType type, const void* data, size_t size);

  virtual const char* Name() const;
  virtual bool Array() const;

  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);

  PointerProxy Data() const;
  PointerProxy operator*() {
    return Data();
  }
  PointerProxy Original() {
    return Data();
  }

  hash_t GetResourceHash() const;
  void Deallocate();

  virtual uint64_t Size() const override;

protected:
  hash_t _resource_hash;
  std::vector<char> _data;
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
  virtual int ElementCount() const {
    return N;
  }
  // Payload size equals sum of wrapped element sizes (no count header emitted)
  virtual uint64_t Size() const override {
    uint64_t total = 0;
    for (int idx = 0; idx < N; ++idx) {
      T_WRAP wrapper_(_array[idx]);
      total += wrapper_.Size();
    }
    return total;
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

  // Payload size: 32-bit element count followed by each element serialized
  // via its wrapper T_WRAP (for non-floating types) or raw bytes (for floats).
  virtual uint64_t Size() const override {
    uint64_t total = sizeof(uint32_t);
    if (_array.empty()) {
      return total;
    }

    if constexpr (std::is_floating_point<T>::value) {
      // For floats we write raw bytes in Write(), so account for them here
      total += static_cast<uint64_t>(_array.size()) * sizeof(T);
    } else {
      for (const auto& elem : _array) {
        T_WRAP wrapper_(elem);
        total += wrapper_.Size();
      }
    }
    return total;
  }
};

/**
  * @brief Wrapper for an array of arguments
  *
  * gits::CArgumentSizedArray is divided using partial template specialization into
  * default CArgumentSizedArray<T, T_WRAP> and CArgumentSizedArray<char, T_WRAP>
  */
template <class T, class T_WRAP, class T_GET_MAPPED_POINTERS = uint64_t>
class CArgumentSizedArray : public CArgument {
  CArgumentSizedArrayBase<T, T_WRAP> _sizedArray;

public:
  CArgumentSizedArray(size_t num = 0) : _sizedArray(num) {}
  template <size_t N>
  CArgumentSizedArray(const T (&array)[N]) : _sizedArray(N, array) {}
  CArgumentSizedArray(size_t num, const T* array) : _sizedArray(num, array) {}
  CArgumentSizedArray(const std::vector<T>& array) : _sizedArray(array) {}
  CArgumentSizedArray(const T* array, T terminator, int term_pos)
      : _sizedArray(array, terminator, term_pos) {}
  CArgumentSizedArray(const T* ptr) : _sizedArray(1, ptr) {}

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

  uint64_t Size() const override {
    return _sizedArray.Size();
  }

  virtual std::set<T_GET_MAPPED_POINTERS> GetMappedPointers() {
    return std::set<T_GET_MAPPED_POINTERS>();
  }
};

template <class T_WRAP>
class CArgumentSizedArray<char, T_WRAP> : public CArgument {
  CArgumentSizedArrayBase<char, T_WRAP> _sizedArray;

public:
  CArgumentSizedArray(size_t num = 0) : _sizedArray(num) {}
  template <size_t N>
  CArgumentSizedArray(const char (&array)[N]) : _sizedArray(N, array) {}
  CArgumentSizedArray(size_t num, const char* array) : _sizedArray(num, array) {}
  CArgumentSizedArray(const std::vector<char>& array) : _sizedArray(array) {}
  CArgumentSizedArray(const char* array, char terminator, int term_pos)
      : _sizedArray(array, terminator, term_pos) {}
  CArgumentSizedArray(const char* array) : _sizedArray(array, 0, 1) {}

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

  virtual std::set<uint64_t> GetMappedPointers() {
    return std::set<uint64_t>();
  }

  // Size equals null pointer when empty; otherwise pointer + count + bytes
  virtual uint64_t Size() const override {
    if (_sizedArray.Vector().empty()) {
      return sizeof(void*);
    }
    return sizeof(void*) + sizeof(uint32_t) + sizeof(char) * _sizedArray.Vector().size();
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
enum MappedArrayAction {
  ADD_MAPPING = 0,
  REMOVE_MAPPING = 1,
  NO_ACTION = 2
};

template <class T, class T_WRAP, MappedArrayAction T_ACTION>
class CArgumentMappedSizedArray : public CArgument {

  enum ValuesType {
    MAPPEDS = 0,
    ORIGS,
    NULLS,
  };

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

    ProxyArray(const ProxyArray& other) = delete;
    ProxyArray& operator=(const ProxyArray& other) = delete;
    ProxyArray(std::vector<T>& arr, std::vector<T>& mappedarr, MappedArrayAction action)
        : _mappedArray(&mappedarr), _array(&arr), _action(action) {}
    ~ProxyArray() {
      try {
        if (_action == ADD_MAPPING) {
          // Add mapping
          for (size_t i = 0; i < _array->size(); i++) {
            T_WRAP::AddMapping((*_array)[i], (*_mappedArray)[i]);
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
        if (T_WRAP::CheckMapping(elem) || _action != ADD_MAPPING) {
          (*_mappedArray)[i] = T_WRAP::GetMapping(elem);
        } else if (T_WRAP::InitializedWithOriginal()) {
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
  size_t ElementCount() const {
    return _array.size();
  }
  void RemoveMapping() {
    for (size_t i = 0; i < _array.size(); i++) {
      T_WRAP::RemoveMapping(_array[i]);
    }
  }

  std::vector<T>& Vector() {
    return _array;
  }
  const std::vector<T>& Vector() const {
    return _array;
  }

  virtual const char* Name() const {
    return T_WRAP::NAME;
  }
  virtual void Write(CBinOStream& stream) const;
  virtual void Read(CBinIStream& stream);

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

  // Proper payload size: pointer sentinel + count + per-element payloads
  virtual uint64_t Size() const override {
    // Matches Write(): we emit a 32-bit size header, then each element via T_WRAP
    uint64_t total = sizeof(uint32_t);
    for (const auto& elem : _array) {
      // T_WRAP wrapper serializes the element
      T_WRAP wrapper_(elem);
      total += wrapper_.Size();
    }
    return total;
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
  virtual uint64_t Size() const override;
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

  size_t Count() const {
    return _cStringTable.size();
  }
  const size_t* Lengths() const {
    return _lengthsArray.data();
  }
  virtual std::set<uint64_t> GetMappedPointers() {
    return std::set<uint64_t>();
  }
  // Size equals element count header + sum of inner Cchar::CSArray sizes
  virtual uint64_t Size() const override {
    uint64_t total = sizeof(uint32_t);
    for (const auto* arg : _cStringTable) {
      if (arg) {
        total += arg->Size();
      }
    }
    return total;
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
        _cargs[i] = std::make_shared<TKeyArg>();
        _cargs[i]->Read(stream);
      }
    } else {
      throw std::runtime_error(EXCEPTION_MESSAGE);
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

  virtual uint64_t Size() const override {
    uint64_t sz = sizeof(uint32_t); // element count
    for (const auto& arg : _cargs) {
      if (arg) {
        sz += arg->Size();
      }
    }
    return sz;
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

  virtual uint64_t Size() const override {
    uint64_t sz = sizeof(uint32_t); // outer count
    for (const auto& outer : _cargs) {
      sz += sizeof(uint32_t); // inner count
      for (const auto& inner : outer) {
        if (inner) {
          sz += inner->Size();
        }
      }
    }
    return sz;
  }
};

//************************** Cint ***********************************
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
  virtual uint64_t Size() const override;
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
  virtual uint64_t Size() const override;
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
  virtual uint64_t Size() const override;
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
  virtual uint64_t Size() const override;
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
  virtual uint64_t Size() const override;
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
  virtual uint64_t Size() const override;
};

//************************** Cint8_t ***********************************
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
  virtual uint64_t Size() const override;
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
  virtual uint64_t Size() const override;
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
  virtual uint64_t Size() const override;
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
  virtual uint64_t Size() const override;
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
  virtual uint64_t Size() const override;
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
  virtual uint64_t Size() const override;
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
  virtual uint64_t Size() const override;
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

  void* Original() const;
  void* Value() const;
  void* operator*() const;

  std::set<uint64_t> GetMappedPointers();
  virtual uint64_t Size() const override;

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
  if (!Configurator::Get().common.recorder.nullIO) {
    stream.write((char*)&size, sizeof(size));
  }

  if (size != 0) {
    static constexpr bool isfloat = std::is_floating_point<T>::value;
    if constexpr (!isfloat) {
      for (unsigned idx = 0; idx < size; idx++) {
        T_WRAP wrapper_(_array[idx]);
        stream << wrapper_;
      }
    } else {
      // floats never need to adjust their type
      stream.write((const char*)&_array[0], size * sizeof(T));
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
      static constexpr bool isfloat = std::is_floating_point<T>::value;
      if constexpr (!isfloat) {
        for (auto idx = 0U; idx < size; idx++) {
          T_WRAP wrapper_;
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
  if (!Configurator::Get().common.recorder.nullIO) {
    stream.write((char*)&size, sizeof(size));
  }

  if (size != 0) {
    for (unsigned idx = 0; idx < size; idx++) {
      T_WRAP wrapper_(_array[idx]);
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
      T_WRAP wrapper_;
      stream >> wrapper_;
      _array[idx] = wrapper_.Original();
    }
  }
}

template <class T, int N, class T_WRAP>
void gits::CArgumentFixedArray<T, N, T_WRAP>::Read(CBinIStream& stream) {
  for (unsigned idx = 0; idx < N; idx++) {
    T_WRAP wrapper_;
    stream >> wrapper_;
    _array[idx] = *wrapper_;
  }
}

template <class T, int N, class T_WRAP>
void gits::CArgumentFixedArray<T, N, T_WRAP>::Write(CBinOStream& stream) const {
  for (int idx = 0; idx < ElementCount(); idx++) {
    T_WRAP wrapper_(_array[idx]);
    stream << wrapper_;
  }
}
