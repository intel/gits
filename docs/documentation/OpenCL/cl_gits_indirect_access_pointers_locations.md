---
icon: simple/opengl
title: Indirect Access Pointers Locations
---

# Notice {#_notice}

Copyright (c) 2023 Intel Corporation.

# Overview {#_overview}

This extension offers API helper function call that allows GITS to
translate indirect use of pointers inside USM allocation. Extension is
exposed by GITS recorder only, without any communication to the driver.
It preferably could be a special application mode, that will allow to
record proper traces. Extension availability can be checked by querying
via API extension function getter. Application **must** use the API call
before NDRange kernel submit. This way GITS Player shall translate every
occurrence of pointer inside USM allocation before kernel execution.

# New API Functions {#_new_api_functions}

``` c++
void clGitsIndirectAllocationOffsets(
void *pAlloc,
uint32_t numOffsets,
size_t *pOffsets
);
```

# Extension usage {#_extension_usage}

Availability can be queried via:

``` c++
void* clGetExtensionFunctionAddressForPlatform (
  cl_platform_id platform,
    const char *funcname
);
```

The function

``` c++
void clGitsIndirectAllocationOffsets(
void *pAlloc,
uint32_t numOffsets,
size_t *pOffsets
);
```

saves information to GITS stream about indirect access pointers
locations that will be translated into Player address space during
playback.

*pAlloc* is the pointer to an USM memory allocation where data contains
indirect pointers addresses. It **must** be the value returned by a call
responsible for USM creation.

*numOffsets* is the number of offsets inside pOffsets.

*pOffsets* \[range(0, numOffsets)\] is a pointer to the array of
offsets, where each offset specify beginning of the indirect address
inside pAlloc. The addresses might be pointers to USM regions.

# Sample pseudocode {#_sample_pseudocode}

``` c++
struct IndirectData {
  void *usmPtr;
  char randomData[1024];
  void *usmPtrs[6];
};

int main() {
  const IndirectData pAlloc = initialize_data();
  if (clGetExtensionFunctionAddressForPlatform(
          platform, "clGitsIndirectAllocationOffsets") != nullptr) {
    std::vector<size_t> offsetVector;
    const auto begin = reinterpret_cast<uintptr_t>(&pAlloc);
    offsetVector.push_back(reinterpret_cast<uintptr_t>(&pAlloc.usmPtr) - begin);

    for (auto i = 0; i < sizeof(pAlloc.usmPtrs) / sizeof(void *); i++) {
      const size_t offset =
          reinterpret_cast<uintptr_t>(&pAlloc.usmPtrs[i]) - begin;
      offsetVector.push_back(offset);
    }
    clGitsIndirectAllocationOffsets(&pAlloc,
                                    static_cast<uint32_t>(offsetVector.size()),
                                    offsetVector.data());
  }
  return 0;
}
```
