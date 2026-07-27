---

icon: material/head-snowflake-outline
title: How-To

---

GITS can be configured in multiple ways:

- a yaml configuration file (generated during the configuration code generation phase) 'gits_config.yml'.
- through command line arguments passed to the `gitsPlayer`.
- through setting (automatically generated) environment variables.

The system prints out which configuration values are changed, the old and new value as well as the method used.

This page describes how values can be set and the remainder of this part of the documentation describes all configuration options. The methods are exemplified with the same two settings.

# Yaml File

The generated configuration file `gits_config.yml` contains the correct full structure and all default values. The system is able to read configuration files with a subset of values (a _partial configuration_), e.g. only changed values. Entries of the configuration file that can not be matched will be printed out in the log.

Here's a simple _partial configuration_ in which we set the player to be in _fullscreen_ and _stop playback after Frame 23_:

```YAML
Common:
  Player:
    Fullscreen: true
    StopAfterFrames: 23
```

# Environment Variables

_Every_ option can be set by an environment variable in the form of the full keypath in the Yaml file with prefix `GITS_` and `_` as seperators:

```bash
export GITS_COMMON_PLAYER_STOPAFTERFRAMES=23
export GITS_COMMON_PLAYER_FULLSCREEN=true
```

# Command Line Arguments

!!! Note
    For obvious reasons only the `gitsPlayer` can be configured using the commandline.  
    In the following examples we're ommitting the stream to be played back for readability. The correct usage of `gitsPlayer` is
    ```bash
    gitsPlayer {OPTIONS} [path to stream]
    ```

GITS provides a regular command line interface. For our example settings used above there are dedicated arguments defined:

```bash
gitsPlayer --stopAfterFrames=23

# it's a flag, so presence ==> true
gitsPlayer -f
gitsPlayer --fullscreen
```

## Hidden Arguments

Apart from the _regular_ argument command line interface GITS also has _hidden arguments_, that allow to set any configuration value. The _hidden argument_ is created from the full keypath in the Yaml file, e.g.:

```bash
gitsPlayer --Common.Player.StopAfterFrames=23

gitsPlayer --Common.Player.Fullscreen # flag again
```

## Setting flags to false

Flags are always set a value to **`true`**, but sometimes a value needs to be set to false explicitly. For this reason there's a _hidden argument_ for every **bool value** that consists of the full keypath with the suffix ".Value" which can be set to either `0` (meaning `false`)or `1` (meaning `true`):

```bash
gitsPlayer --Common.Player.Fullscreen.Value=0 # Set the fullscreen set to false
gitsPlayer --Common.Player.Fullscreen.Value=1 # Set the fullscreen set to true
```

## Setting arrays

Some GITS options consist of an array, e.g. the built-in HUD can be enabled for an array of APIs:

```bash
gitsPlayer --Common.Shared.HUD.Enabled=DX    # Enable HUD for DX only
gitsPlayer --Common.Shared.HUD.Enabled=DX,VK # Enable HUD for DX and VK
gitsPlayer --Common.Shared.HUD.Enabled=[]    # Disable HUD completely
```

