---
icon: octicons/file-binary-24
---
## Binary stream

### Format

Binary GITS streams are collections of files produced by GITS recorder.
Some of those resources may not be created by GITS recorder, depending
on feature set of application being recorded.

#### Current stream format

Current GITS version will produce binary stream where following
files/directories are created:

-   File `stream.gits2`  
		Main stream file, containing persisted tokens.

-   File `gitsSignature.hash`  
		Signature file storing hashes of all stream components.

-   File `gitsBuffers.dat`  
		Binary data storage.

-   File `gitsTextures.dat`  
		Binary data storage.

-   File `gitsClientSizes.dat`  
		Auxiliary, lazily persisted data.

-   File `gitsDataIndex.dat`  
		Index storing mappings of hashes to file storage mappings.

-   File `gitsPrograms.zip`  
		Zip file containing shaders used by the application.

-   Directory `gitsPrograms`  
		Directory with shader files. If this directory exists, its content
		has priority over zipped shaders.

### Stream integrity

Together with created binary stream, GITS recorder will create a
signature file `gitsSignature.hash` that contains hashes of the stream
files. This file can be then used to make sure that stream is not
corrupted when playing back. For verification use GITS player with
`verifyStream` option, which will perform stream verification and exit
or play the stream back respectively.

Stream verification is normally very fast, so unless stream file is
excessively large it provides essentially free, extra safety against
environmental problems (like stream corruption during copying from
remote share).

### Additional diagnostic info

During recording GITS gathers various diagnostic information that can be
useful later on. This information can be divided in two groups: generic
and system specific information.

To retrieve this info from stream use player option `recorderDiags` .
Apart from information about architecture of recorded application, GITS
version, GITS config used, system specific information is gathered.

On Windows various system information is obtained through WMI services.

### Streams portability

Streams recorded on one system are not tied only to this particular
configuration when playing it back. Gits player gives user a huge degree
of portability not only between platform driver or OS but also on the
API level.

#### OpenGL

Binary OpenGL/ES streams are portable in five dimensions:

streams recorded from x86, x64 and ARM applications are playable on any
of two rest architectures.

streams recorded on one hardware may be played back on any other
hardware if only this hardware supports used API version and extensions.

stream may be played on any other system than it was recorded if only
this system supports used API version and extensions.

gitsPlayer has a built in translator which makes conversions between
WGL, GLX and EGL APIs in arbitrary direction. While playing back stream
on non OS different than one used to capture the stream, gitsPlayer
automatically chooses best native API for this OS and stream. Native API
may be forced through `forceGLNativeAPI` option. User has to be aware
that cross native API translation is based on some simplifications
because native APIs differ a lot and some features available in one API
may not be available in other APIs. One of the most substantial
simplifications concerns pixel format selection. During conversion only
key attributes related to color/depth/stencil and buffering are taken
into account.

Because there are a lot of similarities between OpenGL types and
versions GITS player gives user some degree in portability also in this
area. Nevertheless such conversions may cause crashes if any API or
shader used in stream is not supported in forced API. Forcing Opengl
profile (CORE, COMPAT or ES) is possible through `forceGLProfile`
option. API version may be forced through `forceGLMinorVersion` and
`forceGLMajorVersion` options.

#### Compute

Binary Compute (OpenCL/RS) streams support only limited portability:

streams recorded from x86 and ARM applications are playable on one
another. x64 streams are incompatible with other architectures.

as in OpenGL streams recorded on one hardware may be played back on any
other hardware if only this hardware supports API version and
extensions.

if only architecture match as described above and used API version and
extensions are supported streams are portable between OSes.

#### Vulkan

Binary Vulkan streams support only very limited portability due to the
low-level nature of the API:

streams should be portable between different operating system versions
as long as appropriate Vulkan driver is available. Streams recorded on a
given hardware/driver configuration are only guaranteed to be properly
played back on a similar hardware/driver configuration. To mitigate this
problem a set of recorder options was introduced, which increases the
portability of recorded streams. Using them should allow for replaying
streams on configurations other than the one used during recording,
though the compatibility isn't guaranteed. This comes with a cost of a
modified behavior of the recorded application, higher memory consumption
and/or potentially lower performance. The following options can be used
to increase stream's cross-platform compatibility:

#### Issues and solutions

Because of differences between OSes and APIs streams porting is a very
fragile process. It may lead to unexpected crashes or corruptions which
in some cases may be easily worked around. In such a case it is a good
idea to try to run stream on destination configuration with following
cmd
`gitsPlayer.exe <stream path> --logLevel TRACE --traceGLError > log.txt 2>&1`
and explore the log looking for errors like `Err: *` or
`Trace: GL Error *`. Below are common issues and solutions:

\- If log contains `Trace: GL Error: GL_INVALID_OPERATION` after shader
program loading/compilation API calls (for i.e. glCompileShader or
glProgramStringARB) it may mean that line endings in shader files are
not supported. To solve the problem extract shaders from
gitsPrograms.zip to gitsPrograms folder and use
`<GITS FOLDER>\UtilityScripts\ShadersConvertEOLtoLF.py` script to change
line endings to LF.

\- If stream is crashing with error saying that certain function
couldn't be loaded it means that this particular API is not supported on
the current configuration. In many cases APIs that are not supported
does not influence rendering and may be removed or may be replaced by
another equivalent API (for example extension to core API version). To
do it prepare a proper lua script or ask GITS team for help.

