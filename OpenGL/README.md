# Modern OpenGL Backend

This directory contains the work-in-progress new OpenGL backend. The previous implementation lives
under `OpenGLLegacy/` and remains operational alongside it.


It follows a layered architecture approach pioneered by the DirectX and new Vulkan backends.

The backend is 64-bit only. It is built by default (`WITH_OPENGL`) on Windows x64 (WGL) and
x86-64 Linux (GLX). EGL and possibly 32-bit support may be added later.

Files ending in `Auto` are generated. Edit the corresponding Mako template or Python generator in
`codegen/` and regenerate instead of editing generated output.