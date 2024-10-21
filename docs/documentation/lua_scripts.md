---
icon: simple/lua
---
## Lua scripts {#sec:Lua}

### Introduction

Both recorder and player have capability of instrumentation of GL and CL
functions (and limited instrumentation of Vulkan functions) via external
script. If the script defines an OpenGL (and EGL, WGL, GLX) or OpenCL,
or Vulkan function, GITS will call that function, instead of the one
provided by the driver. It is then script's responsibility to forward it
to the driver (if it chooses to do so). This can be done to modify the
stream in a specific way. For example to cause all `glDrawElements`
functions to render `GL_POINTS`, one could use a script as follows.

    GL_POINTS = 0x0000
    function glDrawElements(mode, count, type, indices)
      drv.glDrawElements(GL_POINTS, count, type, indices)
    end

### Event hooks

GITS also allows for definition of non API-specific, specially named
functions that will be called at specific times during stream playback.

-   `gitsFrameBegin(no)`  
    Called at the beginning of a frame. `<no>` will be number of frame which
    is just starting.

-   `gitsFrameEnd(no)`  
    Called at the end of a frame. `<no>` will be number of frame that
    just finished playing back.

-   `gitsLoopBegin(no)`  
    Called at beginning of `<no>` iteration of stream loop.

-   `gitsLoopEnd(no)`  
    Called at end of `<no>` iteration of stream loop.

-   `gitsStateRestoreBegin()`  
    Called at beginning of GITS state restoration phase.

-   `gitsStateRestoreEnd(no)`  
    Called at end of state restoration phase.

-   `gitsProgramStart()`  
    Called when GITS starts.

-   `gitsProgramExit()`  
    Called just before GITS exits.

### API hooks

Apart from these, GITS has capability to call any API via structures:

-   `drv` for OpenGL,

-   `drvCl` for OpenCL,

-   `drvVk` for Vulkan.  
    Note: Vulkan API does not support structures (structure members
    cannot be used or modified inside scripts).

### Utility functions

There is a number of intrinsic functions that GITS exports for some
operations normally impossible in Lua. All pointers are represented as
opaque Lua userdata.

-   `gits.udtToStr(udt)`  
    Converts `<udt>` pointer to a Lua string and returns it. This can be
    used to print or copy a C string in Lua.

-   `gits.allocUdtFromStr(string)`  
    Allocates data where it stores passed lua string as null terminated
    C-string. Returned data should be deallocated using freeBytes.

-   `gits.udtToInt(udt)`  
    Converts `<udt>` pointer to integer and returns it.

-   `gits.getUdt(arr, idx)`  
    `<arr>` is an array of pointers. This function returns pointer from
    index `<idx>`. Index is zero based.

-   `gits.getByte(arr, idx)`  
    `<arr>` is an array of bytes. This function returns a byte from
    index `<idx>`. Index is zero based.

-   `gits.getInt(arr, idx)`  
    `<arr>` is an array of integers. This function returns integer from
    index `<idx>`. Index is zero based.

-   `gits.getInt64(arr, idx)`  
    `<arr>` is an array of integers. This function returns integer from
    index `<idx>`. Index is zero based.

-   `gits.getSizeT(arr, idx)`  
    `<arr>` is an array of integers. This function returns integer from
    index `<idx>`. Index is zero based.

-   `gits.getFloat(arr, idx)`  
    `<arr>` is an array of floats. This function returns float from
    index `<idx>`. Index is zero based.

-   `gits.setUdt(arr, idx, val)`  
    `<arr>` is an array of pointers. This function sets a pointer from
    index `<idx>` to value of `<val>`. Index is zero based.

-   `gits.setByte(arr, idx, val)`  
    `<arr>` is an array of bytes. This function sets a byte from index
    `<idx>` to value of `<val>`. Index is zero based.

-   `gits.setInt(arr, idx, val)`  
    `<arr>` is an array of integers. This function sets an integer from
    index `<idx>` to value of `<val>`. Index is zero based.

-   `gits.setInt64(arr, idx, val)`  
    `<arr>` is an array of integers. This function sets an integer from
    index `<idx>` to value of `<val>`. Index is zero based.

-   `gits.setSizeT(arr, idx, val)`  
    `<arr>` is an array of integers. This function sets an integer from
    index `<idx>` to value of `<val>`. Index is zero based.

-   `gits.setFloat(arr, idx, val)`  
    `<arr>` is an array of floats. This function sets a float from index
    `<idx>` to value of `<val>`. Index is zero based.

-   `gits.nullUdt()`  
    Returns a null pointer.

-   `gits.currTime()`  
    Returns amount of microseconds since GITS start.

-   `gits.msgBox(title, message, type)`  
    Calls MessageBoxA WinAPI forwarding passed parameters to it. Noop on
    non-Windows platforms.

-   `gits.getPtrSize()`  
    Returns pointer size in bytes.

-   `gits.allocBytes(size)`  
    Allocates `<size>` bytes and returns pointer to the allocation.

-   `gits.freeBytes(udt)`  
    Frees `<udt>` which should be a value returned from
    gits.allocBytes().

-   `gits.getArgsStr()`  
    Returns string passed to `scriptArgsStr` player option.

-   `gits.gitsGlDebugProc()`  
    Returns a pointer to a function that is compatible with GL debug
    procedure and outputs its parameters. This is only useful for
    setting GL debug proc via Lua.

-   `gits.getOutDir()`  
    Returns path GITS output directory set via `outputDir` option.

-   `gits.isInt64InRange(begin, size, number)`  
    Checks if 64-bit `<number>` is a part of region starting from
    `<begin>` of size `<size>`. Overcomes Lua limitation on 64-bit
    Integers operations.

-   `drvCl.statusToStr(status)`  
    Converts `cl_int` return code to string.

    #### OpenCL callbacks

    Some OpenCL calls allow to pass a callback function which will be
    called asynchronously by the driver when certain event occurs.

    Due to the way Lua bridge works, you can't pass Lua functions as
    `pfn_notify` parameter directly. However, GITS provides a few
    functions to support passing Lua functions as callbacks:

-   `drvCl.getEventCallbackPtr()`

-   `drvCl.getMemObjCallbackPtr()`

-   `drvCl.getProgramCallbackPtr()`

-   `drvCl.getContextCallbackPtr()`

-   `drvCl.getSVMFreeCallbackPtr()`

    These functions return UDT pointers to C callback function with
    appropriate signature. They should be passed as `pfn_notify`.

-   `drvCl.setCallbackData(function, user_data)`  
    Returns UDT which should be passed as `user_data` parameter. It
    contains references to Lua `function` that should be called and UDT
    that will be passed to Lua callback as `user_data` .

    Example usage is shown in `callbackExample.lua` script bundled with
    GITS.

### Example

    local GL_SCISSOR_TEST = 0x0C11
    local GL_SCISSOR_BOX = 0x0C10
    local GL_COLOR_BUFFER_BIT = 0x4000

    -- function performs scissored clear on specified bounds
    function clear_box(x, y, width, height)
      local scissor = gits.allocBytes(4 * 4)

      -- save original scissor enable status
      local enabled = drv.glIsEnabled(GL_SCISSOR_TEST)

      -- save original scissor box
      drv.glGetIntegerv(GL_SCISSOR_BOX, scissor)

      -- perform scissored clear
      drv.glEnable(GL_SCISSOR_TEST)
      drv.glScissor(x, y, width, height)
      drv.glClear(GL_COLOR_BUFFER_BIT)

      -- extract original scissor bounds
      -- from native pointer
      local s0 = gits.getInt(scissor, 0)
      local s1 = gits.getInt(scissor, 1)
      local s2 = gits.getInt(scissor, 2)
      local s3 = gits.getInt(scissor, 3)
      drv.glScissor(s0, s1, s2, s3)

      -- restore original scissor enable
      if enabled == 0 then
        drv.glDisable(GL_SCISSOR_TEST)
      end

      gits.freeBytes(scissor)
    end

### Included scripts {#sec:includedLuaScripts}

GITS comes with a set of scripts for common use cases.

#### OpenCL

-   `clconstants.lua`  
    Contains Lua definitions of OpenCL constants supported by GITS.
    Usage: `import "clconstants"`.

-   `aubCapture.lua`  
    Executes specified batch scripts before and after
    `clEnqueueNDRangeKernel` call.

-   `buildLog.lua`  
    Prints build log after `clBuildProgram`.

-   `callbackExample.lua`  
    Example script demonstrating callback usage.

-   `forceDeviceType.lua`  
    Forces requested or reported device type.

-   `forceNDRangeWorkSize.lua`  
    Forces work size for `clEnqueueNDRangeKernel` calls.

-   `forcePlatformVendor.lua`  
    Forces platform vendor in `clGetPlatformInfo`.

-   `forceReportedGlobalMemSize.lua`  
    Forces global memory size and max allocation size for all devices.

-   `forceReportedWorkGroupSize.lua`  
    Forces work group size for all devices.

-   `injectBuildOptions.lua`  
    --scriptArgsStr: build options to inject  
    Injects options to `clBuildProgram` and `clCompileProgram` calls.

-   `nullNDRange.lua`  
    Avoids sending `clEnqueueNDRangeKernel` to the driver. Useful when
    trying to capture kernel execution that hangs.

-   `performanceGITS.lua`  
    Dumps performance data to CSV file in backwards-compatible format.

-   `dumpPerformanceData.lua`  
    More sophisticated script to dump performance data. It does not
    inject blocking calls.

-   `printReturnCodes.lua`  
    Prints statuses from OpenCL functions which return them through
    `errcode_ret` parameter.

#### Vulkan

-   `vulkanKeepDraws.lua`  
    Performs draw calls filtering (executes only those drawing functions
    that are in a specified range).

-   `vulkanLogNumDraws.lua`  
    Counts and displays draw calls order numbers (useful for
    `vulkanKeepDraws.lua` script).

