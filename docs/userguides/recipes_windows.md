---
icon: material/microsoft-windows
---
## Windows {#windows-3}

### Frames all stream from application {#itm:framesAllWindows}

- Install GITS
- Determine recorded process architecture and pick correct GITS version (32 vs 64 bit). You can find this information in Task Manager:
    - Run target application
    - Right-click the taskbar
    - Start Task Manager
    - Go to Processes
    - Find target application and determine process architecture
    - Exit the application
- Please choose and copy proper directory content to target application directory (one containing application exe file):
    - If you are working only with OGL API it should be directory `Recorder\FilesToCopyOGL\`
    - If you are working with OGL and OCL API it should be directory `Recorder\FilesToCopyOCL\`
    - If you are working with GLES API it should be directory `Recorder\FilesToCopyES\`
    - If you are working with Vulkan API it should be directory `Recorder\FilesToCopyVulkan\`
- Make sure you have default values in gits_config.txt.
- Run the application and perform actions to be recorded
- Exit the application
- Recorded stream is located in `<GITS DIR>\dump\<process id>_<year>_<month>_<day>_<hour>_<minute>_<second>.<millisecond>\`

### Single frame subcapture {#itm:singleFrameSubcaptureWindows}

- Install GITS
- Please choose and copy proper directory content to gitsPlayer directory:
    - If you are working only with OGL API it should be directory `Recorder\FilesToCopyOGL\`
    - If you are working with OGL and OCL API it should be directory `Recorder\FilesToCopyOCL\`
    - If you are working with GLES API it should be directory `Recorder\FilesToCopyES\`
    - If you are working with Vulkan API it should be directory `Recorder\FilesToCopyVulkan\`
- Make sure you have default values in gits_config.txt.
- Modify copied gits_config.txt:
    - In section OpenGL.Capture set Mode to Frames
    - In section OpenGL.Frames set StartFrame and StopFrame
    - Save above changes
- Replay frames all stream from application using gitsPlayer
- Recorded stream is located in `<GITS DIR>\dump\<process id>_<year>_<month>_<day>_<hour>_<minute>_<second>.<millisecond>\`

### Cross-platform compatible Vulkan single frame subcapture {#itm:crossplatformCompatibleVulkanSingleFrameSubcaptureWindows}

The below tutorial shows how to record a sub-stream that should be compatible with different Intel Gen architectures. For example, the below steps should allow for a sub-stream to be recorded on a Skylake-family processor and played back on a Cannolake-family processor.

- Install GITS
- Determine recorded process architecture and pick correct GITS version (32 vs 64 bit). You can find this information in TaskManager:
    - Run target application
    - Right-click the taskbar
    - Start Task Manager
    - Go to Processes
    - Find target application and determine process architecture
    - Exit the application

- Copy `Recorder\FilesToCopyVulkan\` directory contents to target application directory (one containing application exe file)
- Make sure you have default values in gits_config.txt
- Modify copied gits_config.txt:
    - In section `Vulkan.Utilities` set value of the `IncreaseImageMemorySizeRequirement` parameter to 64000
    - Save above changes
- Run the application and perform actions to be recorded
- Exit the application
- Recorded stream is located in `<GITS DIR>\dump\<process id>_<year>_<month>_<day>_<hour>_<minute>_<second>.<millisecond>\`
- Copy `Recorder\FilesToCopyVulkan\` directory contents to gitsPlayer directory
- Make sure you have default values in gits_config.txt.
- Modify copied gits_config.txt:
    - In section `Vulkan.Capture` set value of the `Mode` parameter to Frames
    - In section `Vulkan.Capture.Frames` set desired values of the `StartFrame` and `StopFrame` parameters
    - Save above changes
- Replay recorded frames all stream using gitsPlayer
- Recorded sub-stream is located in `<GITS DIR>\dump\<process id>_<year>_<month>_<day>_<hour>_<minute>_<second>.<millisecond>\`

### Single frame ccode

- Install GITS
- Please choose and copy proper directory content to gitsPlayer directory:
    - If you are working only with OGL API it should be directory `Recorder\FilesToCopyOGL\`
    - If you are working with OGL and OCL API it should be directory `Recorder\FilesToCopyOCL\`
    - If you are working with GLES API it should be directory `Recorder\FilesToCopyES\`
    - If you are working with Vulkan API it should be directory `Recorder\FilesToCopyVulkan\`
- Make sure you have default values in gits_config.txt.
- Modify copied gits_config.txt:
    - In section Basic set BinaryDump to False and CCodeDump to True
    - Save above changes
- Replay single frame stream using gitsPlayer
- Go to (`<GITS DIR>\dump\<process id>_<year>_<month>_<day>_<hour>_<minute>_<second>.<millisecond>\`)
- Use `build_ccode.bat` to compile files.

