# Modern OpenGL Backend

This directory contains the work-in-progress new OpenGL backend. `OpenGL2` is a temporary directory
name used while the old backend under `OpenGL/` remains operational. Once the new backend is
considered complete enough, it will be moved to `OpenGL/` and the old backend to `OpenGLLegacy/` or
a similarly named folder.


It follows a layered architecture approach pioneered by the DirectX and new Vulkan backends.

Initially, only 64-bit Windows (WGL) will be supported. Later, Linux and EGL/GLX support will be
added. Possibly also 32-bit support.

Files ending in `Auto` are generated. Edit the corresponding Mako template or Python generator in
`codegen/` and regenerate instead of editing generated output.