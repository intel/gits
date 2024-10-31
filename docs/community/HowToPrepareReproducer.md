---
icon: octicons/bug-24
---
**GITS** is a tool which allows to intercept and serialize APIs called by the application with the intent of later playback. One of the use cases is ability to reproduce driver or HW issues affecting an application by running just a series of API calls, without the original software stack above. 

This short guide explains only how to prepare a **GITS** stream based reproducer. Features unrelated to performing this task are not covered by this document.
	

**GITS** supports the following APIs:

* Compute: OpenCL, Level Zero, DirectML
* Render: OpenGL, Vulkan, Direct3D 12
	

**GITS** recorder is based on the shared library being injected into the process of recorded application.

Capturing API stream on Linux:

* Initial assumption for default settings:
  * Application shouldn't crash during the recording. 
  * Application is recorded from start to the end.
* Set `LD_LIBRARY_PATH` environment variable to folder containing **GITS** intercepting library for selected API. For example `export LD_LIBRARY_PATH=<GITS_DIR_PATH>/Recorder/OpenCL` or `export LD_LIBRARY_PATH=<GITS_DIR_PATH>/Recorder/Vulkan`.
* Run application and make sure that it exits gracefully.
* The captured stream should appear in the `<GITS_DIR_PATH>/dump/` folder. Captured stream folders include PID of the captured process in their names.

Captured stream verification:

* Try to run the captured stream to check if it is valid, by running `<GITS_DIR_PATH>/Player/gitsPlayer <CAPTURED_STREAM_FOLDER>`. Stream should run and finish gracefully.
* Run stream with `--logLevel TRACEVERBOSE` to get log with all called API calls in the output.
* Run stream with dumping buffers/images/textures/frames. It allows to check if output buffers/images are as expected. Check option description in `gitsPlayer` help for more details (e.g., run `gitsPlayer -hh all`). Examples:
    * **Open**CL - use `--clCaptureKernels 1-10` to dump buffers and images used by executed kernels from the first to tenth kernel (artifacts are being dumped to `dump` subfolder of the stream folder).
    * **LevelZero** - use `--l0Capture` options to dump buffers and images used by executed kernels, with selected granularity.
    * **Render APIs** - use `--captureFrames` to dump framebuffer per frame.


Troubleshooting:

> TODO - This seems not correct!

* Recorded application is crashing (even without attaching GITS) and captured stream is getting corrupted - Modify gits_config.txt file (`<GITS_DIR_PATH>/Recorder/<API>/gits_config.txt`) by setting `HighIntegrity` to `True`. This option makes **GITS** terminating stream on each captured API call and in case of crash it remains consistent. This option heavily affects recording performance.
* Recorded application crashes on start while being recorded with **GITS**. Check gits_config.txt file, if it contains proper paths to libraries being intercepted. Correct them if needed. Example: `Basic.Paths.LibCL`.
* No stream is getting captured:
  * Make sure that recorded application is same architecture (x86 or x64) as recorder.
  * On Linux try alternative interception method. Use symbols preloading: `LD_PRELOAD=<GITS_DIR_PATH>/Recorder/<API>/<INTERCEPTED_LIB>` instead of `LD_LIBRARY_PATH` based method.
