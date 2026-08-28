---
icon: material/head-question-outline
title: About GITS
---

# Graphics Intercept and Trace Solution (GITS)

**Graphics Intercept and Trace Solution** (**GITS**) is an opensource capture-replay tool by [Intel](https://www.intel.com) for  

- [Vulkan :fontawesome-solid-arrow-up-right-from-square:](https://vulkan.org/)
- [DirectX12 :fontawesome-solid-arrow-up-right-from-square:](https://learn.microsoft.com/en-us/windows/win32/direct3d12/direct3d-12-graphics)
- [OpenCL :fontawesome-solid-arrow-up-right-from-square:](https://www.khronos.org/opencl/)
- [Intel oneAPI Level Zero :fontawesome-solid-arrow-up-right-from-square:](https://spec.oneapi.io/level-zero/latest/core/INTRO.html)
- [OpenGL :fontawesome-solid-arrow-up-right-from-square:](https://www.khronos.org/opengl/)

**GITS** allows you to record sequences of API calls into binary traces that can be replayed later (we call them '*streams*'). See the [Usage section](#usage) for more info.

```mermaid
flowchart LR

    A(Application):::app ---> R(gitsRecorder):::gits
    R ---> SR[(Stream)]:::stream

    P ---> D(Display):::show
    P ---> Res(Resources):::res

    subgraph GITS ["`**GITS**`"]
    C{{Configuration}}:::conf -.-> R
    C{{Configuration}}:::conf -.-> P
    end
    SP[(Stream)]:::stream ---> P(gitsPlayer):::gits

    classDef strokeStyle stroke:#fff,stroke-width:2px,color:#fff
    classDef gits fill:#006FC5
    classDef app fill:#C40500
    classDef show fill:#38C400
    classDef stream fill:#254F6F
    classDef res fill:#00C4B1
    classDef conf fill:#C400B3

    class A,R,SR,P,D,Res,E,,SP,C strokeStyle

    classDef gitsSubgraphStyle stroke:#006FC5,stroke-width:2px,color:#fff,fill:#006FC580,stroke-dasharray:5
    class GITS gitsSubgraphStyle
```

# Target audience

**GITS** is a collection of *command line tools* bundled with a *launcher* which has been used for years to help develop and validate *Intel GPU drivers*, but we think it can be useful to other users as well.

!!! note
    If you are a game developer who wants to analyze frames using a graphical tool we'd recommend to use other tools, such as [RenderDoc](https://renderdoc.org/), [PIX](https://devblogs.microsoft.com/pix/) or a similar tools.

# Install & Building

Currently we do not provide prebuilt binaries, so you have to [build it yourself](building.md). **GITS** is written in C++ so you will need a *compiler*, a *build system* and *Python 3*. 

# Usage

To **record an application**, you will have to inject our dynamic library (called '*the interposer*') into it. On *Windows*, this is typically done by copying a DLL into the app directory. On *Linux* by manipulating loader environment variables. When recording *Vulkan* it is also possible to use **GITS** as a *Vulkan layer* instead.

To **replay a stream**, pass it as an argument to the **gitsPlayer**-executable.

To simplify the usage we've created the [GITS Launcher](usingLauncher.md), a GUI application that takes care of the required steps for you as well as setting up GITS options.
