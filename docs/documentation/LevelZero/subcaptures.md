---
icon: simple/intel
title: Subcapture
---
Subcapture is a GITS stream that contains selected subset of API calls.
It preserves application functionality (input and output data is the
same) but reduces its scope. Feature supports complicated scenarios when
kernel is being used multiple times inside one or more command list or
kernel is reusing the same region of USM allocation. When playing the
subcapture trace back, GITS will initialize all buffers used by a kernel
to the state they were during recording the application.

Level Zero is working on Command Queues and Command Lists. Kernels are
appended into a Command List and lists into a Command Queue. GITS during
recording can't determine how many kernels there will be inside a
command list. As a result, an additional pre-numbering logic is
required.

Subcapture not always contain all of the application's API calls. The
goal is to select minimal portion of the application behavior.

# Activating subcapture mode

Level Zero GITS subcapture mode is activated by changing the
`gits_config.txt` file, `LevelZero` section. `LevelZero.Capture.Mode`
must be set to `Kernel` and desired kernel range must be chosen:
`LevelZero.Kernel.Range`.

# Nomenclature

The main nomenclature is
`CommandQueueSubmitRange/CommandListRange/AppendKernelsRange`.

`CommandQueueSubmitRange` - Range of command queue executions. Every
`zeCommandQueueExecuteCommandLists` and `zeCommandListImmediateAppendCommandListsExp` API call GITS increments the
value.[^1]

`CommandListRange`- On every `zeCommandListCreate` or
`zeCommandListCreateImmediate` API call GITS increments the value.[^1]

`AppendKernelsRange` - On every family of append kernel to command list
i.e.
`zeCommandListAppendLaunchKernel, zeCommandListAppendLaunchCooperativeKernel, zeCommandListAppendLaunchKernelIndirect, zeCommandListAppendLaunchMultipleKernelsIndirect`
GITS increments the value

[^1]: 
    In case of Immediate Command Lists, on `zeCommandListCreateImmediate` API call GITS reserve(increment) the number of CommandQueueSubmitRange as well as CommandListRange.

Example of specifying kernel range `LevelZero.Capture.Kernel` to the
`1/3/4`. That means the first API call of
`zeCommandQueueExecuteCommandList` contain third application created
command list, which contains kernel, which was appended as fourth append
launch kernel API for example: `zeCommandListAppendLaunchKernel`. The
numbering logic can be looked up inside the recording log on every
kernel execution. `Basic.LogLevel` must be set to at least `TRACE`.
Recommended solution is to verify the numbering with a recorder's log
after whole application was successfully recorded.

# Range subcapture

It is possible to record a subcapture of the given range. To create
range subcapture specify start and end points of the subcapture.
Everything within these points will be recorded. The nomenclature for
range subcapture is as follows
`StartCommandQueueSubmit-StopCommandQueueSubmit/StartCommandList-StopCommandList/StartAppendKernel-StopAppendKernel`.

## Limitations

-   It is only possible to specify only one start and one end point for
    the subcapture. For instance, a kernel range such as
    `1-3,6/3-5,7-8/4-20` would be invalid.

-   When recording a subcapture of multiple command queues the end
    kernel has to be the last kernel in the end command list. For a
    detailed example, refer to [Example 3](#example-3).

-   A range subcapture cannot be created where the start kernel is
    greater than the stop kernel.

Warning: Range subcapture can cause GITS to use a significant amount of
memory. This could potentially lead to crashes in some workloads.

# Numbering examples

## Example 1:

``` c++
// CommandQueueSubmit = 0; CommandList = 0; AppendKernels = 0;
zeCommandListCreate(cmd_list_1); // CommandList++ (1)
zeCommandListCreate(cmd_list_2); // CommandList++ (2)
zeCommandListCreate(cmd_list_3); // CommandList++ (3)
zeCommandListAppendLaunchKernel(cmd_list_2, kernel_1); // AppendKernels++ (1)
zeCommandListAppendLaunchKernel(cmd_list_2, kernel_2); // AppendKernels++ (2)
zeCreateCommandQueue(cmd_queue_1);
zeCommandQueueExecuteCommandLists(cmd_queue_1, {cmd_list_2}); // CommandQueueSubmit++ (1)
```

Subcapture creation of the first execution of kernel_2:

`LevelZero.Capture.Kernel 1/2/2`

## Example 2:

``` c++
// CommandQueueSubmit = 0; CommandList = 0; AppendKernels = 0;
zeCommandListCreate(cmd_list_1); // CommandList++ (1)
zeCommandListCreate(cmd_list_2); // CommandList++ (2)
zeCommandListCreate(cmd_list_3); // CommandList++ (3)
zeCommandListAppendLaunchKernel(cmd_list_2, kernel_1); // AppendKernels++ (1)
zeCommandListAppendLaunchKernel(cmd_list_2, kernel_2); // AppendKernels++ (2)
zeCommandListCreateImmediate(cmd_list_4); // CommandList++, CommandQueueSubmit++ (4), (1)
zeCreateCommandQueue(cmd_queue_1);
zeCommandQueueExecuteCommandLists(cmd_queue_1, {cmd_list_2}); //CommandQueueSubmit++ (2)
zeCommandListAppendLaunchKernel(cmd_list_4, kernel_1); // AppendKernels++ (3)
zeCommandListAppendLaunchMultipleKernelsIndirect(cmd_list_4, {kernel_3, kernel_4}); //
// AppendKernels++, AppendKernels++ (4), (5) (the numbering of multiple kernels is the same order that they were listed)
zeCommandListAppendLaunchKernelIndirect(cmd_list_4, kernel_3); // AppendKernels++ (6)
```

To create a subcapture of the second execution of kernel_3 inside the
cmd_list_4(immediate):

`LevelZero.Capture.Kernel 1/4/6`

## Example 3:

``` c++
// CommandQueueSubmit = 0; CommandList = 0; AppendKernels = 0;
zeCommandListCreate(cmd_list_1); // CommandList++ (1)
zeCommandListCreate(cmd_list_2); // CommandList++ (2)
zeCommandListCreate(cmd_list_3); // CommandList++ (3)
zeCommandListAppendLaunchKernel(cmd_list_1, kernel_1); // AppendKernels++ (1)
zeCommandListAppendLaunchKernel(cmd_list_1, kernel_2); // AppendKernels++ (2)
zeCommandListAppendLaunchKernel(cmd_list_2, kernel_3); // AppendKernels++ (3)
zeCommandListAppendLaunchKernel(cmd_list_3, kernel_4); // AppendKernels++ (4)
zeCreateCommandQueue(cmd_queue_1);
zeCreateCommandQueue(cmd_queue_2);
zeCreateCommandQueue(cmd_queue_3);
zeCommandQueueExecuteCommandLists(cmd_queue_1, {cmd_list_1}); //CommandQueueSubmit++ (1)
// Queue #1 / CommandList #1 / Kernel #1
// Queue #1 / CommandList #1 / Kernel #2
zeCommandQueueExecuteCommandLists(cmd_queue_2, {cmd_list_1, cmd_list_3}); //CommandQueueSubmit++ (2)
// Queue #2 / CommandList #1 / Kernel #1
// Queue #2 / CommandList #1 / Kernel #2
// Queue #2 / CommandList #3 / Kernel #4
zeCommandQueueExecuteCommandLists(cmd_queue_3, {cmd_list_2}); //CommandQueueSubmit++ (3)
// Queue #3 / CommandList #2 / Kernel #3
```

To create subcapture from Queue #2 / CommandList #1 / Kernel #1 to Queue #3 / CommandList #2 / Kernel #3 set:

`LevelZero.Capture.Kernel 2-3/1-2/2-3`

To create subcapture from Queue #2 / CommandList #1 / Kernel #2 to Queue #2 / CommandList #3 / Kernel #4 set:

`LevelZero.Capture.Kernel 2/1-3/2-4`

Creating a subcapture from Queue #1 / CommandList #1 / Kernel #1 to
Queue #2 / CommandList #1 / Kernel #1 is not feasible. This is because
the end point is in the middle of the command list and the range
includes multiple command queues. Attempting to do so could result in
undefined behavior.

# Recording subcapture by kernel name and execution number 

<span style="font-size:30px;">:writing_hand: TODO</span>
