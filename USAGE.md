# Usage of GITS

## API and hardware support

GITS is being tested mostly on Intel hardware. GPUs from other manufacturers are not guaranteed to work.

GITS development is focused on enabling applications, not APIs or extensions. We implement support for API calls when we encounter apps using them. This means it's possible that a fancy new feature is implemented, while an old feature isn't. It's possible that an extension defines multiple functions, but only one of them is supported by GITS. It's possible that a feature works in some apps, but not in others.

Nevertheless, here is an *approximate* list of what's supported:
- OpenGL up to 4.3 (Notable exceptions: `GL_ARB_buffer_storage` and `GL_ARB_vertex_attrib_binding` are implemented, but have known issues. Some other features are unimplemented.)
- Vulkan 1.0, 1.1, 1.2 plus several extensions
- OpenCL up to 3.0 (Note: CCode is unsupported)
- Intel oneAPI Level Zero (Note: CCode is unsupported) - Additional L0 documentation is available in `doc/LevelZero/` folder, e.g. [overview of available options](doc/LevelZero/options_features.asciidoc).

TL;DR your mileage may vary, we don't guarantee that your use case will work.


## Recorder

### General info

The recorder part of GITS consists of two types of dynamic libraries: recorder proper and interceptors. There is a separate interceptor for each API, but there is only one recorder that handles all the APIs and can even record multiple APIs at once, where supported.

Recorder configuration is stored in a file named `gits_config.txt`, which we refer to as "config file" or simply, "config". GITS will look for it in the same place where the interceptors are located, or in the directory specified with `GITS_CONFIG_DIR` environment variable (if set). Interceptors need to read recorder's location from the config file. The config file provides many options affecting the recording process, which are described inside the config file itself.

Recorder operates by creating a _token_ for each recorded API call in the thread which invoked that call. Token contains the information necessary to play back the API function it represents. Recorder then passes each newly created token to a separate thread, which serializes the token to disk.

Newly created streams will appear in the dump directory. By default it's inside your GITS install. You can change it using the `DumpDirectoryPath` setting.

**Recorder creates a log file. Read it, paying attention to errors.** It will save you a lot of headache. Its location varies per platform, see the OS-specific section.

### OS-specific info

Due to differences in platforms supported by GITS, recording process is slightly different on all supported platforms from user perspective.

#### Windows

Windows API interception mechanism uses Windows loader's feature that causes it to search for DLLs in the folder from which the application loaded, before searching in system directories.

This means that to correctly record any application, appropriate GITS interceptor DLLs need to be placed in the directory where target EXE file is located and GITS DLLs architecture must match that of target EXE file.

* Determine *recorded process'* architecture (32- vs 64-bit) and install the corresponding GITS version.
* Determine the APIs used by the target application and open the relevant recorder directory (e.g. `Recorder\FilesToCopyVulkan` if the app uses Vulkan).
* Copy the contents of aforementioned recorder directory (config file and DLLs) to the target application directory containing the `.exe` file. Ensure you have write access to the app directory, as a log file will be created there.
* Run the application and perform actions to be recorded.
* Exit the application.
* Find the dumped stream in the `dump` subdirectory of your GITS install and a log file in the target application directory. **Read the log file, it will list any errors, saving you time and stress.**

If you are working with OpenGL ES (GLES) API instead of desktop OpenGL, copy **only** the contents of the `FilesToCopyES` directory. OpenGL32.dll **must not** be present in the app directory in that case.

##### Recording applications which provide their own API DLLs

If application delivered its own API DLL, there is a possibility that some calls in it have been adjusted for that specific application’s use. You will have to rename the original API DLL and put the interceptor DLL in its place. For example, let's do it with OpenCL.dll:

1. Find out whether application uses its own OpenCL.dll. Go to application’s directory and scan it for OpenCL.dll.
2. If you found it, change its filename: OpenCL.dll -> OpenCL_Original.dll.
3. Copy GITS interceptor DLL and config file to this directory.
4. Open gits_config.txt, find OpenCL section and change `Basic.Paths.LibCL` to the full path of OpenCL_Original.dll. Make sure to escape the backslashes (`\\` instead of `\`).

If the interceptor still cannot attach to the app, and you are sure you did everything correctly (check the app vs GITS architecture), see the next section.

##### Recording applications which directly open DLLs from System32

Those applications need special handling because the loader will only load DLLs from the exact path given by the app (e.g., `C:\Windows\System32\OpenCL.dll`), so the usual method won't work in these cases. To record such an application, you will need to use a trick much like the one described in the section above.

First, rename the real OpenCL.dll. Then, copy the interceptor DLL and the config file to the system directory (On 64-bit system: System32 for 64-bit apps, SysWOW64 for 32-bit apps; on 32-bit system: System32). After this, path to the original OpenCL.dll needs to be adjusted in the config (i.e. Basic.Paths.LibGL) to point to the renamed file.

##### OpenCL guidelines

###### Saving streams

To be sure that desired device is captured, you can use one of included Lua scripts.

You can also use the scripts to specify work group size. This might be useful if stream is being captured on newer device but must be played later on an older one. If stream needs to be played on an older platform than is available for recording, forced work group size should be set to that of older platform, because streams recorded with bigger work group size will crash when played on a machine with smaller work group size.

There are two ways of approaching this issue:
* Capture one stream with forced work group size, so it will work on all platform with specified lower work group size.
* Capture more streams, one for direct work group size platform with default 0 setting.

When subcaptures are using CL/GL sharing calls and you don't want them to, you need to re-record them with `Extras.Utilities.RemoveAPISharing` set to "OGL" or "All". You can check that by playing recorded stream with –s option. Look for these functions:
* `clEnqueueAcquireGLObjects`
* `clEnqueueReleaseGLObjects`

If they are not present, replacing OpenGL DLLs is not necessary. That way tool’s bugs associated with OpenGL can be avoided.

However, you should not use `Extras.Utilities.RemoveAPISharing` and `OclSingleKernel` mode simultaneously. Instead, record a full stream using `RemoveAPISharing` and then record a single kernel stream (without using `RemoveAPISharing`) from that full stream.

User should never stop recording stream by forcing application to quit – that may corrupt the recorded stream. You can specify special keystrokes to stop the recorder cleanly – find the `Basic.ExitKeys` option in config file.

If stream is recorded with OpenGL calls, `OpenGL.Capture.All.ExitFrame` option can be used. It will quit the application after specified amount of frames recorded. Adjust it using trial and error method. For this method to work OpenGL DLL has to be replaced with GITS one.

###### Common issues

If any problem occurs when recording a stream, try to record the application using only OpenCL.dll, then using both OpenCL.dll and OpenGL.dll. That means at first only OpenCL.dll should be replaced, OpenGL DLLs should be left untouched. Then user should check if issue occurs in both streams or in just one. This should indicate if issue is on GITS OCL part or OGL part.

If application is causing issues with GITS recorder, but is working fine without it, tracing should be enabled to see which call is causing GITS to crash.

Any encountered bugs should be reported. Consult the [Issues section](#issues).

Sometimes GITS will crash during very long recordings. `Extras.Optimizations.TokenBurstLimit` might help in such cases.

#### Linux

##### Regular applications

1. Check `gits_config.txt` for your chosen API and edit it if necessary (for example, library paths might be different on your distro)
2. Set the environment variable `LD_LIBRARY_PATH`:
    * for OpenCL workload: `export LD_LIBRARY_PATH=<GITS_DIR_PATH>/Recorder/OpenCL`
    * for OpenGL workload: `export LD_LIBRARY_PATH=<GITS_DIR_PATH>/Recorder/OpenGL`
    * for Vulkan workload: `export LD_LIBRARY_PATH=<GITS_DIR_PATH>/Recorder/Vulkan`
    * for LevelZero workload: `export LD_LIBRARY_PATH=<GITS_DIR_PATH>/Recorder/LevelZero`
3. Run the application
4. Remove recording environment variable: `unset LD_LIBRARY_PATH`
5. Find the dumped stream in the `dump` subdirectory of your GITS install and a log file in the target application directory. **Read the log file, it will list any errors, saving you time and stress.**

##### Steam applications

Steam overwrites environment variables passed to games, so setting them normally won't work. Fortunately, you can set set them in custom launch options for a given Steam game. Syntax looks like this: `LD_LIBRARY_PATH=<GITS_DIR_PATH>/Recorder/OpenGL %command%`. Steam will replace `%command%` with game's executable.

Some games, like Dota 2, might overwrite `LD_LIBRARY_PATH` with their own. In this case the above method won't work and you'll have to improvise. (For example, use `LD_PRELOAD` or edit game launcher scripts.)

### Vulkan layer

In addition to the usual method of recording, Vulkan apps can also be recorded by using GITS as a Vulkan layer.

On Windows, the layer is registered in the Windows Registry by CMake's install target, if CMake is run with elevated permissions. Then, it can be enabled using the usual mechanisms like Vulkan Configurator or `VK_INSTANCE_LAYERS` environment variable.

On Linux, the layer is not registered and you need to point to it using the `VK_LAYER_PATH` environment variable. You also need to enable the layer using the `VK_INSTANCE_LAYERS` environment variable. In summary it looks like this:

```bash
# Replace the placeholder with the absolute path to your GITS install.
export VK_LAYER_PATH=<GITS install path>/Recorder/VulkanLayer/
export VK_INSTANCE_LAYERS=VK_LAYER_INTEL_vulkan_GITS_recorder

# Now you can run your application:
./your_app
```


## Player

### General info

Player (`gitsPlayer`) is a command-line tool that allows you to replay binary streams. You need to point it either to a folder containing a stream, or to a `.gits2` file inside this folder. Use player's `--help` option to see basic usage info, or `--hh all` to list all available options.

To see statistics of a given stream, use the `--stats` option. To dump rendered frames to disk, use the `--captureFrames` option; for example `--captureFrames 25,100-200:10` will capture frame 25 and every tenth frame between frames 100 and 200. To dump draws, use `--captureDraws`, which works similarly to `--captureFrames`. Dumped images will appear in a subdirectory of the stream folder.


## Binary streams

We call them "binary streams" or just "streams". Other tools call them "dumps" or "traces". They are sequences of serialized API calls saved to disk by the recorder and can be replayed using the player.

OpenGL, OpenCL and Level Zero streams should be portable between different machines. Vulkan streams in general are not portable because of Vulkan's low level nature. Expect them to work only on the machine they were recorded on, anything else is a fortunate exception. Even then, something as trivial as a driver update might break compatibility.

### Substreams

Substreams are binary streams that contain only a subset of calls made by the application, for example only 1 frame. To create them, you need to record the player (like you would record any other app) with appropriate recorder settings (for example, `Mode Frames`, `StartFrame 100`, `StopFrame 100`).

We refer to binary streams that are not substreams as "full streams".


## CCode

CCode is what we call C++ projects generated by the recorder. Instead of being replayed by the player, they need to be built and executed instead.

### Recording

To record CCode, follow the steps for recording substreams with the following exceptions: in recorder's config file, change `BinaryDump` to `False`, `CCodeDump` to `True`, and ensure `Mode` of relevant API is set to `All`. To keep the size of generated code reasonable and build times low, we recommend limiting the number of frames by recording CCode from player playing a substream with only few frames.

At the end of recording, CCode project files are being copied, which can slow down or even hang the recorded application. Don't kill it, even if it's unresponsive; please wait instead.

### Building

To build CCode, use CMake. On Windows, you can also use the `build_ccode.bat` script. Run it without arguments to see the available options. On Linux, you can run the `build.linux` script.

To avoid building Boost again and again for each CCode project you generate, which can take a long time, you can use the `-DBOOST_ROOT` CMake option. It accepts a path to Boost code that was already built, for example, `-DBOOST_ROOT="<path to GITS repo>/third_party/boost_x64"`.


## Issues

### FAQ

Q: OpenGL stream recorded on one platform is being played on another platform and gitsPlayer shows errors related to context or pixel format selection.

A: Try playing the stream with the `minimalConfig` option.

Q: Recorder is crashing.

A: Analyze recorder log. (See the [recorder section](#recorder) for more info.) It may contain hints on what to do. For example: "Mixed Framebuffers calls (EXT and Core). Consider enabling ScheduleFboEXTAsCoreWA in recorder config.".

Q: Overly high memory consumption when replaying a stream, stream replay crashes.

A: Replay the stream with the `--tokenBurstLimit <arg>` gitsPlayer option to limit the total number of data (stream tokens) loaded to memory by the player. Provide some low value as the option's argument - 10000, 2000 or even less. Please note that the smaller the value, the lower the playback performance (due to much more frequent storage access).

### Streams are not being finished properly
If player refuses to play a stream, showing the `Signature file not found.` error, this usually means the game/app process was terminated before recorder could finish writing the stream to disk. (The signature file creation is the last stage of dumping a stream.) The premature process termination may be caused either by a crash or by other reason such as the app killing its own processes when quitting takes too long. Relevant information may appear in the recorder log, in the app's own log or in the app's console output. There are multiple solutions that might be helpful:
  - Detach the recorder before the app quits or the crash happens. There are multiple ways to do so, depending on your operating system. Recorder options for detaching include:
    - `Basic.ExitKeys`
    - `Basic.ExitSignal`
    - `Basic.ExitAfterAPICall`
    - `*.Capture.All.ExitFrame`
    - `OpenGL.Capture.All.ExitDeleteContext`
    - `Extras.Utilities.ForceDumpOnError` which may help, but it may also cause an abort or other serious problems. Please try other detaching options first.
    - Setting a `*.Capture.Frames.StopFrame`, `OpenGL.Capture.OglDrawsRange.StopDraw`, `*.Number` and similar API-specific recorder options used for recording substreams (capture modes other than `All`).
  - Disable `Extras.Utilities.CloseAppOnStopRecording` recorder option. In rare cases this option may cause the app to terminate before the stream is fully dumped. This problem is known to affect Pyre, Blender, and SPECviewperf.
  - Enable the `Extras.Utilities.HighIntegrity` mode. This option forces the recorder to persist the stream to disk after every API call. Unfortunately it has a heavy impact on recorder performance. Sometimes the resulting stream may still lack the signature file, but otherwise be fine. (For example when the app process was terminated during the calculation of signatures.) In this case you can try manually signing the stream as described below.
  - As a last resort, you can try manually signing the stream using the player's `--signStream` option. This is unlikely to help, so you are advised to exhaust other options first. Expect the resulting stream to cause crashes during playback.


### Recorder hangs
When the recorded app stops responding, it might not necessarily indicate a problem. Recording a stream decreases the performance of the app, especially the final phase, in which the stream is being finished and signed. While apps should not hang for extended periods during the regular recording phase, they often do hang when recorder is finishing the stream and detaching. In such case it is recommended to wait for it to finish. Depending on the stream size it might take surprisingly long.

However some hangs can be caused by various issues. Known issues include:

Windows:
  - Hang at the start of recording, caused by Windows Management Instrumentation.
    We use WMI to gather information about the system GITS is being used on. We call it "extended diagnostic". Unfortunately some applications also gather data through WMI. When both GITS and the app use WMI at once, it can result in hangs. When it happens, the last message in recorder log should be about WMI. As the log advises, the workaround is to disable the `Extras.ExtendedDiagnostic` option in the recorder config. Known affected apps include Adobe After Effects and 3DMark API Overhead.
