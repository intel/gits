---
icon: octicons/bug-24
---
## Debug Solutions

### Narrowing single frame to single drawcall

In most cases debugging visual corruptions is to narrow the stream to
possibly smallest amount of API calls. First step is capturing single
frame. Such a frame can be often really huge in terms of amount of API
calls and hard to debug. For further narrowing GITS has a single
drawcall recording feature which allows user to subcapture just a single
drawcall (causing corruption) with state narrowed to objects used
(bound) by this drawcall only. It allows significant stream size
reduction even 100 of times.

Step by step:

-   Capture single frame stream reproducing corruption

-   Use --captureDraws option to capture per draw screenshots of draw
    buffer

-   Review captured screenshots - each screenshot has info about the
    draw number and fbo. Find the draw where corruption was introduced
    first time. Be aware that sometimes corruption may come from
    corrupted texture which was rendered previously. It is important to
    find the first draw where corruption appeared. In some cases
    cleanFbos option can be useful. It cleans content of all render
    targets attached to existing frame buffers just before frame
    playback (after state restoration).

-   Set gits recorder to capture selected draw and run single frame to
    make a subcapture.

-   Single draw stream does not give any on screen output but running it
    with captureDraws and captureDrawsPre options will output a
    screenshots from drawbuffer before and after drawcall so effect
    should be visible.

-   To make sure that corruption does not come restored render buffer
    for example run this single draw on some reference where corruption
    shouldn't appear.

-   To play more with captured single draw stream ccode from single draw
    can be captured also.

### Getting drawcalls affecting specified area and filtering drawcalls

Gits has three features which together allows user to find which
drawcalls are drawing to specified area of the frame. Finally it is also
possible to run the stream using only those drawcalls. It can be used as
an alternative for single drawcall recording.

Step by step:

-   Specify area of the frame image to query drawcalls drawing to it.
    Describe it using rectangle with position and size on the queried
    frame.

-   Run stream with `--forceScissor x,y,width,height` option.
    forceScissor option forces scissor with specified values for each
    drawcall excepting clears and blits. It causes that only draws to
    this area are being rendered. Be aware that very often games are
    drawing to inverted fbo and finally are flipping the image during
    blit. In this case scissor rectangle position can be different then
    selected by user. If the result is not desired, per drawcall images
    should be reviewed to better understand how the application is
    drawing.

-   To run the stream with only those drawcalls which drew to specified
    area gitsplayer with `--keepDraws` option has to be used. As a
    parameter to this option list of drawing drawcalls and clears/blits
    should be passed

### Pre-si environment hints

The biggest problem with debugging in a pre-silicon environment is often
the time needed to reproduce the issue. Using certain gits features it
is possible to reduce this time from hours to couple of seconds.

First and easiest step is to render only the part of the frame
containing corruption. This can be done using forceScissor option to
specify and render the corrupted area only. It is significantly reducing
time needed by fulsim. Specified area should be as small as possible to
get a better effect. If from some reasons this approach can't be applied
scaleFactor option can be used to reduce the size of entire window.

Second step can be capturing single drawcall (described in this
chapter). Although, it is sometimes hard to find first draw causing
corruption in presi environment because of the need to capture per draw
images. Option to capture screenshots is using glReadPixels API which
called for hundreds of drawcalls is extremely heavy, takes a very long
time and may not finish on unstable pre-silicon environment. Scissor
option should reduce this time but additional useful step is to get only
drawcalls drawing to specified, corrupted area (described in previous
section). It allows to reduce number of screenshots to be captured on
pre-silicon, to list of drawcalls which could draw the corruption.

