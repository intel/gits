---
icon: octicons/device-camera-24
---
# Recorder

## General information

Recorder configuration is performed through `gits_config.txt` file.
Configuration options stored there affect various facets of operation.
They can be used to specify scope of recording, define additional
channels of communication with recorder (signals), where supported, and
do performance tuning of the recorder.

Recorder operates, by creating a `token` for each recorder function in
the thread that invoked that API function. `Token` represents all the
information necessary to play back function it represents. Recorder then
passes each newly created `token` to a separate thread which is used to
persist data to hdd.

Normally all IO is done in separate threads of execution to minimize
impact on the recorded application. Binary data (texture, buffers) has
to be fingerprinted (hashed) before it can be written to disk, later on
GITS player will reference all binary data through these hashes. Hashing
can become a bottleneck for some workloads. For this reason GITS
provides an option to use sequential numbers as hashes. This allows to
send data to disk much quicker, but will write to disk redundant data if
such is fed to the API by recorded application. This can cause
`gitsTextures.dat`/`gitsBuffers.dat` to become very large.

## Configuration options

Recorder part of GITS is configured by gits_config.txt configuration
file. This file will be searched for in the location of recorder
library, or in the directory specified with `GITS_CONFIG_DIR`
environment variable (if set).

