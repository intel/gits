---
title: Lua Scripting
---

## Overview {#_overview}

This section documents the usage of Level Zero Lua features in **GITS**.

## Features {#_features}

Through the defined **Lua global variable** `drvl0` as an API hook Lua scripts can interact with the a **GITS** stream.

### Modify API call behaviour
You can modify the behavior of an API call with a lua script. Here's an example:

``` lua
function zeEventPoolCreate(hContext, desc, numDevices, phDevices, phEventPool)
  if desc.flags & ZE_EVENT_POOL_FLAG_IPC then
    desc.flags = ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP
  else
    desc.flags = desc.flags | ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP
  end
  return drvl0.zeEventPoolCreate(hContext, desc, numDevices, phDevices, phEventPool)
end
```

### Allocations and deallocations

Level Zero works primarily on **structures** and Lua scripts have the
ability to **read** and **modify** them. Structures within the API call are
automatically converted into Lua Tables and fields can be accessed with
`.` (dot) operator. To access address of a structure there is injected
public field `_self`. There are functions for creating (memory
allocation + initialization): `drvl0.create<structure_name_type>()`, and
deallocating structures: `drvl0.free(<local_lua_variable>)`. 

Raw memory with structure data can be converted to Lua table, by passing the pointer (userdata) as an argument to the function: `drvl0.create\_\<structure_name_type\>(\<pointer\>)`

``` lua
function CreateGitsEventPool(hContext)
  if EVENT_POOL_HANDLE == nil then
    local desc = drvl0.create_ze_event_pool_desc_t()
    local eventPoolPtr = gits.allocBytes(PTR_SIZE)
    desc.stype = ZE_STRUCTURE_TYPE_EVENT_POOL_DESC
    desc.pNext = gits.nullUdt()
    desc.flags = ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP
    desc.count = MAX_NUMBER_OF_EVENTS_PER_EVENT_POOL
    local retVal = drvl0.zeEventPoolCreate(hContext, desc, gits.nullUdt(), gits.nullUdt(), eventPoolPtr)
    if retVal ~= ZE_RESULT_SUCCESS then
      error("zeEventPoolCreate API call couldn't create event pool.")
    end
    EVENT_POOL_HANDLE = gits.getUdt(eventPoolPtr, 0)
    drvl0.free(desc)
    gits.freeBytes(eventPoolPtr)
  end
end
```

## Included scripts

**GITS** comes with various lua scripts in the folder `<gits-root-folder>\Scripts\Lua\LevelZero`:

| | |
|-|-|
|`l0Constants.lua`| Contains Lua definitions of Level Zero constants supported by **GITS**. |
|`dumpPerformanceInfo.lua` | Dumps performance data of each kernel execution to the CSV file. |
