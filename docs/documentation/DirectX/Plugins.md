---
icon: material/microsoft
title: Plugins
---
GITS supports plugins for both capture and replay. GITS plugins are dynamic libraries loaded at runtime by the `gitsPlayer.exe` or `gitsRecorder.dll`. Plugins are included under `Plugins\DirectX\`.

Each plugin consists of the following:

- `gitsPlugin.dll`: Main library containing all the plugin logic.
- `config.yml`: Configuration file (may also contain documentation).
- `dependencies\`: Directory containing all the DLLs the plugin depends on.

# Usage

GITS plugins must be enabled in the configuration file, either under `DirectX.Capture` or `DirectX.Playback`.

```yaml
DirectX:
  Capture:
    Plugins: [] # List of DirectX plugins to enable for Playback
```

```yaml
DirectX:
  Playback:
    Plugins: [] # List of DirectX plugins to enable for Playback
```

To enable a given plugin just add it to the `Plugins[]` list. For example to enable `HelloPlugin` on playback:

```yaml
DirectX:
  Playback:
    Plugins: ['HelloPlugin']
```

## Plugin Names

The plugin name can be found in each plugin's `config.yml` and in the list of plugins [below](#plugin-list).

## Interceptor Only

GITS plugins are very flexible and may not require full capture (or playback) to be enabled. It is possible to disable stream capture and use a GITS plugin without any limitations or errors.

For example, we can enable `HelloPlugin` when running an application without capturing anything by using the following:

```yml
Common:
  Recorder:
    Enabled: false  
```

# Plugin List

The following DirectX plugins are included with GITS.

## HelloPlugin

Example starting point for a DirectX GITS plugin. Annotates presents and GPU executions in the log.

## Benchmark

Creates a .csv file with the present-to-present frame times (CPU) for the stream.

## RtasCache

Caches BLASes through serialization/deserialization by CopyRaytracingAccelerationStructure. This plugin is useful to avoid costly BuildRaytracingAccelerationStructure calls.

## AdapterSpoof

Spoof the DXGIAdapter description returned by `IDXGIAdapter::GetDesc`. This plugin can be used to test how an application behaves for a different GPU vendor (or a specific GPU).


