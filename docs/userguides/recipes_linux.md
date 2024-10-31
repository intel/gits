---
icon: simple/linux
---

## Linux {#linux-3}

### Frames all stream from application {#itm:framesAllLinux}

- Install GITS 
- Make sure you have default values in gits_config.txt.
- Check the correctness of paths to graphics driver libraries in gits_config.txt. Proper settings for Mesa driver:  
  - `LibGL /usr/lib/i386-linux-gnu/mesa/libGL.so.1`
  - `LibEGL /usr/lib/i386-linux-gnu/mesa-egl/libEGL.so.1`
  - `LibGLES1 /usr/lib/i386-linux-gnu/mesa-egl/libGLESv1_CM.so.1`
  - `LibGLES2 /usr/lib/i386-linux-gnu/mesa-egl/libGLESv2.so.2`
- Set recording environment variable:  
  - OpenCL: `export LD_LIBRARY_PATH=<GITS_DIR_PATH>/Recorder/OpenCL/`
  - OpenGL: `export LD_LIBRARY_PATH=<GITS_DIR_PATH>/Recorder/OpenGL/`
  - Vulkan: `export LD_LIBRARY_PATH=<GITS_DIR_PATH>/Recorder/Vulkan/`
- Run the application
- Remove recording environment variable: `unset LD_LIBRARY_PATH`
- Recorded stream is located in `<GITS DIR>\dump\stream-<process id>_<year>_<month>_<day>_<hour>_<minute>_<second>.<millisecond>\`

### Single frame subcapture {#itm:singleFrameLinux}

- Install GITS
- Make sure you have default values in gits_config.txt.
- Check the correctness of paths to graphics driver libraries in gits_config.txt. Proper settings for Mesa driver (section Basic.Paths):  
  - ` LibGL /usr/lib/i386-linux-gnu/mesa/libGL.so.1`
  - `LibEGL /usr/lib/i386-linux-gnu/mesa-egl/libEGL.so.1`
  - `LibGLES1 /usr/lib/i386-linux-gnu/mesa-egl/libGLESv1_CM.so.1`
  - `LibGLES2 /usr/lib/i386-linux-gnu/mesa-egl/libGLESv2.so.2`
- Modify gits_config.txt:
  - In section OpenGL.Capture set Mode to Frames
  - In section OpenGL.Frames set StartFrame and StopFrame
  - Save above changes
- Set recording environment variable:  
  - OpenCL: `export LD_LIBRARY_PATH=<GITS_DIR_PATH>/Recorder/OpenCL/`
  - OpenGL: `export LD_LIBRARY_PATH=<GITS_DIR_PATH>/Recorder/OpenGL/`
  - Vulkan: `export LD_LIBRARY_PATH=<GITS_DIR_PATH>/Recorder/Vulkan/`

- Replay frames all stream from application
- Remove recording environment variable - `unset LD_LIBRARY_PATH`
- Recorded stream is located in `<GITS DIR>\dump\stream-<process id>_<year>_<month>_<day>_<hour>_<minute>_<second>.<millisecond>\`

### Frames all Vulkan stream from Dota 2 Steam game

- Run Steam UI and install 'Dota2 Vulkan support'
- Set proper environment variables used by Steam. To determine which environment variables are used by Steam to run Dota 2 run the following:
  - Run game normally, without GITS, from Steam UI
  - From terminal run: `ps -ef` to get process ID of the running game.
  - From terminal run: `ps eww <process ID>`. It will return command line and set environment variables.
- Run game with properly set environment variables in `/home/<user>/.steam/steam/steamapps/common/dota\ 2\ beta/game/bin/linuxsteamrt64` directory by executing `./dota2 -vconsole -vulkan -console` commandline.

