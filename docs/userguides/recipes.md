---
icon: octicons/milestone-24
---
## General Recipes

### Per drawcall screenshots dumping from stream playback

For dumping screenshots per drawcall use option --captureDraws.
Examples:

- Dump draws 5,7 and 9 `--captureDraws 5,7,9` or `--captureDraws 5-9:2`
- Dump draw number 7 `--captureDraws 7`

### Lua script use case

<span style="font-size:30px;">:writing_hand: TODO</span>

### Draw range subcapture

- Install GITS
- Modify gits_config.txt:  
  - In section `OpenGL.Capture` set `Mode` to `OglDrawsRange`
  - In section `OpenGL.OglDrawsRange` set `StartDraw` and `StopDraw`
  - Save above changes
- Prepare recorder and replay single frame stream

### Single draw subcapture

- Install GITS
- Modify gits_config.txt:  
  - In section `OpenGL.Capture` set `Mode` to `OglSingleDraw`
  - In section `OpenGL.OglSingleDraw` set`Number`
  - Save above changes
- Prepare recorder and replay single frame stream

### Single kernel subcapture

#### Application with OpenGL/DirectX sharing calls

- Install GITS
- Modify gits_config.txt:
  - In section `OpenCL.Capture` set `Mode` to `All`
  - In section `Extras.Utilities` add `"DX"` to `RemoveAPISharing`
  - Save above changes
- Prepare recorder and record the stream
- Replay recorded stream and capture it

#### Application without sharing calls

- Install GITS
- Modify gits_config.txt:
  - In section `OpenCL.Capture` set `Mode` to `OclSingleKernel`
  - In section `OpenCL.Capture.OclSingleKernel` set `Number` to kernel call number you want to capture. You can find it out by recording whole stream with `Basic.CCodeDump` set to `True`, and looking in `stream_frames.cpp`.
  - In section `Extras.Utilities` set `RemoveAPISharing` to `""`
  - Save above changes
- Prepare recorder and record the stream


