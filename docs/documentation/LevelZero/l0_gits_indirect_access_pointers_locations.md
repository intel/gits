---
icon: simple/intel
title: Indirect Access Pointer Locations
---
# Overview {#_overview}

This extension offers API helper function call that allows **GITS** to
translate indirect use of pointers inside allocation. Extension is
exposed by **GITS** recorder only, without any communication to the driver.
It preferably could be a special application mode, that will allow to
record proper traces. Extension availability can be checked by querying
via API extension function getter. Application **must** use the API call
before command buffer submit. This way **GITS** Player shall translate every
occurrence of pointer inside allocation before kernel execution.

# New API Functions {#_new_api_functions}

``` c++
void zeGitsIndirectAllocationOffsets(
void *pAlloc,
uint32_t numOffsets,
size_t *pOffsets
);
```

# Extension usage {#_extension_usage}

**Availability** can be queried via:

``` c++
ze_result_t zeDriverGetExtensionFunctionAddress(
  ze_driver_handle_t hDriver,
  const char* name,
  void** ppFunctionAddress
);
```

The function `zeGitsIndirectAllocationOffsets` below saves information to **GITS** stream about indirect access pointers locations that will be translated into Player address space during playback.

``` c++
void zeGitsIndirectAllocationOffsets(
  void *pAlloc,
  uint32_t numOffsets,
  size_t *pOffsets
);
```



|                                     |                                                                                                                                                                      |
| ----------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| *pAlloc*                            | A pointer to an USM memory allocation where data contains indirect pointers addresses. It **must** be the value returned by a call responsible for USM creation.     |
| *numOffsets*                        | A number of offsets inside pOffsets.                                                                                                                                 |
| *pOffsets* \[range(0, numOffsets)\] | A pointer to the array of offsets, where each offset specify beginning of the indirect address inside pAlloc. The addresses might be pointers to allocation regions. |

## Sample pseudocode {#_sample_pseudocode}

``` c++
struct IndirectData {
  void *usmPtr;
  char randomData[1024];
  void *usmPtrs[6];
};

int main() {
  const IndirectData pAlloc = initialize_data();
  void *pFunctionAddress = nullptr;
  ze_result_t err = zeDriverGetExtensionFunctionAddress(
      hDriver, "zeGitsIndirectAllocationOffsets", &pFunctionAddress);
  if (err == ZE_RESULT_SUCCESS) {
    std::vector<size_t> offsetVector;
    const auto begin = reinterpret_cast<uintptr_t>(&pAlloc);
    offsetVector.push_back(reinterpret_cast<uintptr_t>(&pAlloc.usmPtr) - begin);

    for (auto i = 0; i < sizeof(pAlloc.usmPtrs) / sizeof(void *); i++) {
      const size_t offset =
          reinterpret_cast<uintptr_t>(&pAlloc.usmPtrs[i]) - begin;
      offsetVector.push_back(offset);
    }
    zeGitsIndirectAllocationOffsets(&pAlloc,
                                    static_cast<uint32_t>(offsetVector.size()),
                                    offsetVector.data());
  }
  return 0;
}
```
