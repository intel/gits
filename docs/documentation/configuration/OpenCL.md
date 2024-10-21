---
icon: simple/opengl
---

# OpenCL configuration options

## Capture

- `Mode`  
  Mode of operation of OpenCL capture, allowed values are All, OclSingleKernel, OclKernelsRange.

## OclSingleKernel

- `Number`  
	A clEnqueueNDRangeKernel call number to be captured. Indexing starts at 1.

## OclKernelsRange

- `StartKernel`  
	A clEnqueueNDRangeKernel call number to start capture Indexing starts at 1.

- `StopKernel`  
	A clEnqueueNDRangeKernel call number to stop capture. Indexing starts at 1.

## Utilities

- `DumpKernels`  
	Rangespec with kernels to be captured during the recording.

- `DumpImages`  
	Enables capture OCL images in addition to kernel buffers. Assuming they all are 2D RGBA8 images.

- `OmitReadOnlyObjects`  
	Omits dumping for objects created with CL_MEM_READ_ONLY.

- `BufferResetAfterCreate`  
	Nullifies Buffer, Image, USM and SVM memory regions immediately after their creation to produce deterministic results when verifying buffers. It might inject writes.

- `NullIndirectPointersInBuffer`  
	Nullifies output buffer's indirection pointers in order to produce deterministic results on verification step.

