---
icon: octicons/device-camera-24
title: Recorder
---
# General information

GITS recorder intercepts API calls and stores a binary representation - `token`s - of these calls in the GITS stream file. The file `gits_config.yml` configures various faects of this operation. A `token` represents all information neccessary to play back the function it represents. Alongside the function call itself all other relevant binary data - such as arguments, binary data, etc. are stored. The resulting stream can be played back and will reproduce the recorded API calls.

GITS recorder consists of:

- GITS recorder binary,
- API interceptors and
- optional: *configuration file `gits_config.yml`*, *plugins*.

Details regarding the configuration options can be found in the documentation's [configuration section](../configuration/how_to.md).

For information about the [Vulkan legacy](Vulkan/VulkanLegacy.md) backend, please see the documentation.

# Example usage

--8<-- "recorder_steps.md"


