---
icon: octicons/book-24
---
# General configuration options

## Basic

- **`RecordingMode`**  
	Enables **GITS** recording. Available modes: 

	|            |                                                                    |
	| ---------- | ------------------------------------------------------------------ |
	| `"None"`   | recording disabled, useful for logging API calls without recording |
	| `"Binary"` | enables recording in the format supported by GITS player           |
	| `"CCode"`  | enables recording in C-code format                                 |

- **`LogLevel`**  
	Only info with level greater or equal to this will be logged. API calls are logged at levels `"TRACE"` and lower (lower means more verbose). The available levels are:

	|                  |                                                                                |
	| ---------------- | ------------------------------------------------------------------------------ |
	| `"TRACEVERBOSE"` | *Extremely* detailed output of what the program is doing, suited for developers. |
	| `"TRACE"`        | Detailed output of what the program is doing, suited for more advanced users.  |
	| `"INFO"`         | **Default**. Normal output, usually tells the user what the program is doing.  |
	| `"WARNING"`      | Something potentially bad has happened, the user might want to know.           |
	| `"ERROR"`        | Something bad has happened and user needs to know.                             |
	| `"OFF"`          | Logging is disabled.                                                           |

- **`ExitKeys`** :material-microsoft-windows:  
	If keys specified, after pressing key combination **GITS** plugin will be unloaded from the process. Option available on: Windows (all alpha/special keys)

- **`ExitAfterAPICall`**  
	Enables forcing dumping of **GITS** artifacts after specific number of API calls (0 - do not dump)

- **`ExitSignal`** :simple-linux:  
	Linux only; Signal used to unload **GITS** recorder. Default signal is SIGTERM(15).

### Basic.Paths

- **`LibGL`**  
	Path to library containing actual implementation of OpenGL API.

- **`LibEGL`**  
	Path to library containing actual implementation of EGL API.

- **`LibGLES1`**  
	Path to library containing actual implementation of OpenGL ES 1 API.

- **`LibGLES2`**  
	Path to library containing actual implementation of OpenGL ES 2 API.

- **`LibCL`*  
	Path to library containing actual implementation of OpenCL API.

- **`LibVK`**  
	Path to library containing actual implementation of Vulkan API.

- **`LibOcloc`**  
	Path to library containing actual implementation of Ocloc API.

- **`LibL0`**  
	Path to library containing actual implementation of LevelZero API.

- **`InstallationPath`**  
	Path to **GITS** Recorder directory (containing **GITS** recorder library).

- **`DumpDirectoryPath`**  
	Specified directory will contain newly created stream. 
	
	`%p%` will be substituted with process id of a process that created the stream. 
	
	Windows :material-microsoft-windows: only: `%n%` will be substituted with process name of a process that created the stream.

- **`UniqueDumpDirectory`**  
	The dump directory name for each recorder execution will be postfixed with unique timestamp.


## Extras

### Optimizations

- **`TokenBurstLimit`**  
	The amount of tokens to be used for each recording 'burst'. This many tokens will be accumulated before any IO happens. Packs of that many tokens recorder will be periodically written to HDD in separate thread of execution. Increasing the value will have direct impact on memory used by the recorder. **GITS** will keep a few packs of that many tokens to facilitate hiding of IO latency.

- **`BufferMapAccessMask`**  
	This mask may be used to remove undesired options from buffer mapping access bitfield specified by glMapBufferRange for example. By default **GITS** removes `GL_MAP_INVALIDATE_BUFFER_BIT` and `GL_MAP_INVALIDATE_RANGE_BIT` to improve performance of the recorder. Mask removes those options in recorder so it has an influence on how application and further also stream behaves.

- **`BufferStorageFlagsMask`**  
	This mask may be used to add options to storage flags bitfield specified by glBufferStorage or glNamedBufferStorageEXT. By default **GITS** adds `GL_MAP_READ_BIT`.

### Utilities

- **`ExtendedDiagnostic`**  
	Enables gathering of system diagnostic info during stream recording. Turn off if **GITS** causes application to crash during startup.

- **`ForceDumpOnError`**  
	Enables forcing dumping of **GITS** artifacts when an error is detected. May lead to abort or other serious problems.

- **`ZipTextFiles`**  
	If enabled shaders, kernels and other text files produced by **GITS** will be stored in a zip archives instead of directories.

- **`HighIntegrity`**  
	Forces recorder to operate in high integrity mode, which can heavily impact recorder performance, but will allow for creation of a usable stream, even if the recorded application crashes before end of recording. Stream is effectively written to filesystem after each API call.

- **`NullIO`**  
	Testing mode which does not perform any IO, thus useful only for performance testing of **GITS** recorder itself.

- **`EventScript`**  
	Absolute path to Lua event script to be used during the recording.

- **`RemoveAPISharing`**  
	Removes sharing API calls and replaces it with its 'equivalent' in the pure API. Supported APIs to remove in OpenCL are `"DX"`, `"OGL"`. If set to `"All"` all API sharing calls are removed.

- **`CoherentMapBehaviorWA`**  
	`"WA"` for *Wolfenstein The Old Blood*. This game use extension `ARB_buffer_storage` improperly. It behaves like `MAP_COHERENT_BIT` flag is active, but only `MAP_PERSISTENT_BIT` exist in flags. This `"WA"` cause **GITS** to treat buffer mapping as **COHERENT** also in case of only `MAP_PERSISTENT_BIT` is active (by default all buffers mapped with `MAP_COHERENT_BIT` are updated in each frame by **GITS**).

- **`MTDriverWA`** :material-microsoft-windows:  
	Windows only; Some functions like wglDescribePixelFormat, wglGetPixelFormat, wglSetPixelFormat is used in threaded drivers off the 'main' thread and causes deadlock due to **GITS** api mutex. This WA cause skipping record of these functions when deadlock occurs. In recorder log in this situation you can see warning: `"Deadlock detected in ..."`. Deadlock detection is not perfect and theoretically may cause an unexpected behaviour. Alternative to this WA is disabling multithreading in driver.

- **`CloseAppOnStopRecording`**  
	Forces the recorded application to be closed immediately after the recording process is finished.

- **`WindowsKeyHandling`** :material-microsoft-windows:  
	Select keyboard handling method on Windows:

	|                   |                                                                           |
	| ----------------- | ------------------------------------------------------------------------- |
	| `"MessageLoop"`   | key presses are detected as events returned by GetMessage WinAPI function |
	| `"AsyncKeyState"` | key presses are checked with GetAsyncKeyState WinAPI function             |

### Performance

- **`Benchmark`**   
	Causes **GITS** to produce csv file with system clock times of each frame.