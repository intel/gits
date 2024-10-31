---
icon: simple/intel
---
# Level Zero configuration options

## Capture

- `Mode`  
  Mode of operation of LevelZero capture, allowed values are All, Kernel

- `All`  
  Mode of operation of LevelZero capture. Captures whole stream

- `Kernel`  
  Mode of operation of LevelZero capture. Captures a range of kernels. If chosen one kernel in the range it acts like a single subcapture.  
  Format: **CommandQueueSubmitRange/CommandListRange/AppendKernelsRange**.  
  Each range must exist and may not go out of boundries. For proper numbering see the recorder's log with LogLevel set to at least `TRACE`. Format examples:  
  - 5/5/5  
  - 5,7,8-10/5,7,8-10/5,7,8-10  
  - 1-10:2/1-10:2/1-10:2  

## Utilities

- `DumpKernels`  
  Format QueueSubmitRange/CommandListRange/AppendKernelsRange. For proper numbering understanding look at recorder's log with LogLevel set to at least TRACE. Injects reads with synchronization points. Injected reads are appended into existing command lists. It may cause the host running out of memory, as reads writes into GITS reserved memory space(which GITS holds until queue submit), however, the true gain is the NDRangeBuffer after each executed kernel. If that specific verification is not needed, it is recommended to run this option with DumpAfterSubmit set to True.

- `DumpAfterSubmit`  
  Modifies the way kernel arguments are dumped. Injects immediate command lists, reads and synchronization points after queue submit, instead of injecting read after every appendKernel call. This way dumping process is after command queue execution containing N command lists. If kernel argument is overwritten by many kernels, the output buffer will have the value after last executed kernel that used it. All of other same buffers are skipped. Option reduces allocated memory by GITS.

- `DumpImages`  
  Enables capture L0 images in addition to kernel buffers. Assuming they all are 2D RGBA8 images.

- `BufferResetAfterCreate`  
  Nullifies USM Buffer and Image memory regions immediately after their creation to produce deterministic results when verifying buffers. It might inject writes.

- `NullIndirectPointersInBuffer`  
  Nullifies output buffer's indirection pointers in order to produce deterministic results on verification step.

