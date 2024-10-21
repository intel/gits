---
icon: simple/opengl
---
# OpenGL configuration options


## Capture

- `Mode`  
	Specifies mode of operation and triggering behaviour of GITS recorder. Valid modes are All, Frames, OglSingleDraw, OglDrawsRange. This also selects option group in OpenGL.Capture that is active.


## Capture.All

- `ExitFrame`  
	After this frame, recorder will stop operation and write the stream to filesystem.

- `ExitDeleteContext`  
	Number of context deletion GL function on which to stop recording. Zero means disable feature.

## Capture.Frames

- `StartFrame`  
	First frame to be recorder by GITS recorder.

- `StopFrame`  
	The frame number when recording ends (it will be the last frame recorded). If '0' capture will be ended together with tested application.

- `StartKeys`  
	Key combination beginning capture. If given, capture start and stop are relative to frame in which keypress occurred. If empty, capture start / stop are relative to the beginning of the application. Option available on: Windows (all alpha/special keys).

## Capture.Frames.FrameSeparators

- `glFinish`  
	Specifies glFinish as an additional frame separator. It makes Start/StopFrame values to be counted as a total number of glFinish and SwapBuffers calls.

- `glFlush`  
	Specifies glFlush as an additional frame separator. It makes Start/StopFrame values to be counted as a total number of glFlush and SwapBuffers calls.

## Capture.OglSingleDraw

- `Number`  
	Number of drawcall that is to be recorded. For this draw state minimization will be performed.

## Capture.OglDrawsRange

- `StartDraw`  
	Number of drawcall that will begin stream capture.

- `StopDraw`  
	Number of last draw to be included in the capture.

- `Frame`  
	When set to zero, StartDraw and StopDraw are relative to the beginning of the application and draws range may span many frames. Otherwise it's the number of the frame, whose draws will be captured and StartDraw/StopDraw are relative to the beginning of that frame (only draws belonging to it will be recorded).

## Utilities

- `TraceGLError`  
	If enabled, trace will be amended with error code if any logged function caused an error status. This causes error checking function to be invoked after every logged API call.

- `ForceGLVersion`  
	Overrides the OpenGL version string or number returned to the application. This is useful for changing the behavior of applications.

- `SuppressExtensions`  
	List of extensions to be removed from extension string during recording.

- `SuppressProgramBinary`  
	If enabled, recorded applications that use binary programs/shaders will be forced to fail submitting the binaries to driver, by overriding binary length to 1. This is meant to force application to provide textual shaders definition so that the stream can be played back on other drivers.

- `EndFrameSleep`  
	Sleep this many milliseconds after each frame.

- `RestoreDefaultFB`  
	Option removing restoration of default frame buffer (back or front) content.

- `DoNotRemoveWindow`  
	Option skip scheduling Window removing.

- `MultiApiProtectBypass`  
	Multi API application (for ie those using ES1 and ES2 contexts) are not supported by GITS. Recording such an application will cause an error in gits_xxx.log. Nevertheless it is sometimes possible that such an application may be recorded.

- `CArrayMemCmpType`  
	Specifies the mode of client arrays memory comparison.  

	| | |
	|-|-|
	|**0 - All** | Create diff from whole used memory range.|
	|**1 - One Range** | Create smallest possible one memory range containing all diffs.|
	|**2 - Multi Range** | Create many memory range with diffs.|

- `StripIndicesValues`  
	Ignore specified value of index, when scanning index buffers. For 8 and 16 bit indices, lower order bits are used only.

- `OptimizeBufferSize`  
	If application is using buffers mappings this option causes only changed portion of the buffer to be dumped. Disabling this option causes entire buffer to be dumped on every buffer unmap api call but skips compare operations. From measurements done so far on Intel HW enabling this option improves: buffer size, recording performance and player performance so it should be always on. On other HWs user may consider disabling this option what may improve recorder performance.

- `RetryFunctionLoads`  
	If a pointer to an OpenGL function in the driver cannot be loaded (e.g. because a context has not been created yet), reattempt to load it the next time application calls that function.

- `DetectRecursion`  
	Enable/disable detection of recursion when LibDirectory points to gits library instead of system library.

- `BuffersState`  
	Use to specify buffers restoration type. Set to **CaptureAlways**, **Restore** or **Mixed**.

	| | |
	|-|-|
	|**Capture** | Always - Calls related with buffers are recorded before first frame recording.|
  |**Restore** | Attempt to read data written to write only buffer object mappings. This also allows for state restore of buffer objects in GLES (but is technically undefined). That means calls related with buffers are restored (using state restore), and they are not recorded. Advantage of this method is shorter stream, but this method cannot be used when pixel buffer objects (textures loaded from buffers) are used.|
  |**Mixed** | Merge of restoration type 0 and 1. This method should be work with Transform Feedback objects, and with pixel buffer objects.|

- `TexturesState`  
  Use to specify textures restoration model. Set to **CaptureAlways**, **Restore** or **Mixed**.

	|     |     |
	| --- | --- |
	| **CaptureAlways** | Calls related with textures are recorded before first frame recording.|
	| **Restore** | Enables textures restoration in OpenGLES instead of recording all texture related calls from the beginning. The downside of this option is that it changes texture format what can cause differences in rendered image and fails of texsubimage like calls.|
	| **Mixed** | Merge of restoration type 0 and 1. With this option content of renderable textures with format GL_RGBA and type GL_UNSIGNED_BYTE is being updated in state restoration process.|

- `CoherentMapUpdatePerFrame`  
If set to true, this option enforces updating of the content of buffers, that have been mapped with `MAP_COHERENT_BIT` flag present, only once per frame. Otherwise updates are performed per every drawcall.

- `ScheduleFboEXTAsCoreWA`  
Windows only; Schedule EXT framebuffers calls (extensions: `GL_EXT_framebuffer_object`, `NV_geometry_program4`) as Core calls.

- `UseGlGetTexImageAndRestoreBuffersWhenPossibleES`  
Windows only; Try to use glGetTexImage on OpenGL ES when its exposed by the API.

- `TrackTextureBindingWA`  
Windows only; If set to true, gits relies on tracking of currently bound textures via consecutive `glBindTexture` calls, instead of querying that information from driver with glGetIntegerv.

- `ForceBuffersStateCaptureAlwaysWA`  
Windows only; Affects state restoration process. If enabled, calls related to buffers are recorded before first frame recording.

- `RestoreIndexedTexturesWA`  
Windows only; If enabled, textures, whose format is `GL_COLOR_INDEX` and internal format is `GL_RGBA`, are restored with `glGetTexImage` as being of `GL_RGBA` format.

## Images

- `DumpScreenshots`  
Rangespec with frames to be captured during the recording.

- `DumpDrawsFromFrames`  
Rangespec with frames, from which all drawcalls will be captured.
