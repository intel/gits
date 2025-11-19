---
icon: material/frequently-asked-questions
title: F.A.Q.
---
# Frequently Asked Questions

Q: OpenGL stream recorded on one platform is being played on another platform and gitsPlayer shows errors related to context or pixel format selection.

A: Try playing the stream with the `minimalConfig` option.

Q: Recorder is crashing.

A: Analyze recorder log. It may contain hints on what to do. For example: "Mixed Framebuffers calls (EXT and Core). Consider enabling ScheduleFboEXTAsCoreWA in recorder config.".

Q: Overly high memory consumption when replaying a stream, stream replay crashes.

A: Replay the stream with the `--tokenBurstLimit <arg>` gitsPlayer option to limit the total number of data (stream tokens) loaded to memory by the player. Provide some low value as the option's argument - 10000, 2000 or even less. Please note that the smaller the value, the lower the playback performance (due to much more frequent storage access).

