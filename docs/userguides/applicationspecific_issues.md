---
icon: octicons/apps-24
---
## Application-specific Issues {#chap:ApplicationIssues}

### Blender

OpenGL:

-   If streams are not being finished properly, disable the
    `Extras.Utilities.CloseAppOnStopRecording` recorder option.

### Doom (2016)

OpenGL:

-   Stream replays with visible corruptions - due to application bug,
    record a stream with the `Extras.Utilities.CoherentMapBehaviorWA`
    option set to True.

### Dota 2

OpenGL:

-   The glBufferStorage calls are causing problems in full streams, and
    the `ARB_vertex_attrib_binding` extension is causing corruptions in
    substreams. To fix both, append `GL_ARB_buffer_storage` and
    `GL_ARB_vertex_attrib_binding` (separated by a comma) to
    `OpenGL.Utilities.SuppressExtensions` setting string in recorder
    config. Then, set `OpenGL.Utilities.ForceGLVersion` to \"4.2\". The
    end result should look like this:

        ForceGLVersion "4.2"
        SuppressExtensions "GL_ARB_get_program_binary,GL_ARB_buffer_storage,GL_ARB_vertex_attrib_binding"

### Pyre

OpenGL:

-   If streams are not being finished properly, disable the
    `Extras.Utilities.CloseAppOnStopRecording` recorder option.

Vulkan:

-   Very poor performance when recording a stream on an integrated
    platform - set the `Vulkan.Utilities.ShadowMemory` option to False
    and the `Vulkan.Utilities.MemorySegmentSize` option to 0 (zero).

### Doom Eternal

Vulkan:

-   Very poor performance when recording a stream on an integrated
    platform - set the `Vulkan.Utilities.ShadowMemory` option to False
    and the `Vulkan.Utilities.MemorySegmentSize` option to 0 (zero).

### Red Dead Redemption 2

Vulkan:

-   Very poor recording performance - set the
    `Vulkan.Utilities.ShadowMemory` option to False and the
    `Vulkan.Utilities.MemorySegmentSize` option to 0 (zero).

<!-- -->

-   Rendering errors in a recorded substream - set the
    `Vulkan.Utilities.RestoreMultisampleImagesWA` config option to True
    and record the stream once again.

### Unity3D - Vikings Village techdemo

Vulkan:

-   Rendering errors in a recorded substream - set the
    `Vulkan.Utilities.RestoreMultisampleImagesWA` config option to True
    and record the stream once again.

### Wolfenstein II: The New Colossus

OpenGL:

-   Stream replays with visible corruptions - due to application bug,
    record a stream with the `Extras.Utilities.CoherentMapBehaviorWA`
    option set to True.

Vulkan:

-   Very poor performance when recording a stream on an integrated
    platform - set the `Vulkan.Utilities.ShadowMemory` option to False
    and the `Vulkan.Utilities.MemorySegmentSize` option to 0 (zero).

### Wolfenstein: The Old Blood

OpenGL:

-   Stream replays with visible corruptions - due to application bug,
    record a stream with the `Extras.Utilities.CoherentMapBehaviorWA`
    option set to True.

### 3DMark API Overhead 2.0

Vulkan:

-   During recording, application closes immediately or crashes - due to
    application bug and performance problems, record a stream with the
    `Vulkan.Utilities.ShadowMemory` option set to False and the
    `Vulkan.Utilities.MemorySegmentSize` option set to 0 (zero). After
    that, recapture the stream with default settings (
    `Vulkan.Utilities.ShadowMemory` option set to True and the
    `Vulkan.Utilities.MemorySegmentSize` option set to 512).

### Adobe applications

Adobe applications use OpenGL and usually also OpenCL. Sometimes they
also use DirectX, which GITS does not support. In such case the
`RemoveAPISharing "DX"` recorder option can be used to remove any
DirectX dependencies from the stream being recorded. Details can be
found in the app-specific instructions below.

#### Common steps

-   Download the newest version of GITS from
    <https://validation.igk.intel.com/builds/gits/>

-   Install GITS.

#### After Effects

-   Copy all three files (OGL DLL, OCL DLL, and the config) from
    `"<GITS location>/Recorder/FilesToCopyOCL"` to where After Effects
    is installed (the folder that contains `AfterFX.exe`).

-   Open the copied `gits_config.txt` and in the `Extras -> Utilities`
    section set:

    -   `ExtendedDiagnostic` to `False` . This is to avoid a hang at the
        start.

    -   `RemoveAPISharing` to `"DX"` . This will get rid of DX calls and
        allow us to create a OCL/OGL-only stream.

-   Save the config.

-   Run After Effects and record the stream.

-   Close After Effects. Note that the application might take longer to
    quit than it would normally. Do not end its process prematurely.

-   Play the recorded stream to ensure it was recorded correctly.

##### Notes

-   After Effects uses both OpenGL and OpenCL. We need both of them in
    the stream. This is why we need to copy both `OpenGL32.dll` and
    `OpenCL.dll` to the app directory.

#### Premiere Pro

-   Copy all three files (OGL DLL, OCL DLL, and the config) from
    `"/Recorder/FilesToCopyOCL"` to where Premiere Pro is installed.

-   Open the copied `gits_config.txt` and in the `Extras -> Utilities`
    section set:

    -   `RemoveAPISharing` to `"DX"` . This will get rid of DX calls and
        allow us to create a OCL/OGL-only stream.

-   Save the config.

-   Run Premiere Pro and record the stream.

-   Close Premiere Pro. Note that the application might take longer to
    quit than it would normally. Do not end its process prematurely.

-   Play the recorded stream to ensure it was recorded correctly.

##### Notes

-   Premiere Pro uses both OpenGL and OpenCL. We need both of them in
    the stream. This is why we need to copy both `OpenGL32.dll` and
    `OpenCL.dll` to the app directory.

#### Dimension

-   Copy all three files (OGL DLL, OCL DLL, and the config) from
    `"/Recorder/FilesToCopyOCL"` to where Dimension is installed.

-   Open the copied `gits_config.txt` and in the `Basic` section set:

    -   `ExitKeys` to `"capslock"` . You will use the Caps Lock key to
        detach the recorder from the app, as it won't detach
        automatically when the app closes.

-   Save the config.

-   Run Dimension and record the stream. Detach by pressing the key set
    in config (Caps Lock). Wait a short while so the stream can be
    dumped.

-   Close Dimension.

-   Play the recorded stream to ensure it was recorded correctly. Note
    that no image will appear on the screen, as Dimension does not
    perform buffer swaps. To see the images, please dump them using the
    `–captureFinishFrame` player option, which accepts the same options
    as the regular `–captureFrames` option.

##### Notes

-   When using only few basic features, Dimension did not use OpenCL. We
    copy `OpenCL.dll` just in case other features use OpenCL.

-   Please don't add objects until the viewport is fully loaded. It can
    result in a corrupted stream, which will cause errors like this when
    played:

        Err: Exception thrown when parsing diagnostic information: unexpected end of data
        Err: Unknown token id requested for creation: 0
        Err: Loader reading thread failed: Exiting!!!:failed to create token

#### Other Adobe applications

Other Adobe apps were not tested by the GITS team. We recommend
recording them using the same method as After Effects and Premiere Pro.

