---
icon: material/microsoft
title: Plugins
---
GITS supports plugins for both capture and replay. GITS plugins are dynamic libraries loaded at runtime by `gitsPlayer.exe` or `gitsRecorder.dll`. Built plugins are installed under `Plugins/DirectX/`.

Each plugin folder contains:

- `plugin.dll`: Main library containing the plugin logic.
- `config.yml`: Configuration file (may also contain documentation).
- `dependencies/`: Optional directory with extra DLLs the plugin depends on.

# Usage

Enable plugins under `DirectX.Recorder` or `DirectX.Player` using the `Plugins` list. Each entry must match the name returned by the DLL (`IPlugin::getName()`) which should match the `Info.Name` value in that plugin’s `config.yml`.

```yaml
DirectX:
  Recorder:
    Plugins: [] # List of plugins to enable
```

```yaml
DirectX:
  Player:
    Plugins: [] # List of plugins to enable
```

Example: enable `HelloPlugin` or `HelloHUD` during playback.

```yaml
DirectX:
  Player:
    Plugins: ['HelloPlugin', 'HelloHUD']
```

## Plugin names

Use the **Name** column from the [summary table](#directx-plugins). Full defaults and comments are in each plugin’s `config.yml` beside `plugin.dll`.

## Interceptor only

Plugins can run without recording a stream. Disable the recorder and load a plugin as usual:

```yml
Common:
  Recorder:
    Enabled: false
```

# DirectX plugins

| Name | Folder | Summary |
|------|--------|---------|
| `HelloPlugin` | `hello_plugin` | Example plugin; logs presents and GPU submissions. |
| `HelloHUD` | `hello_hud` | Example DirectX HUD plugin; shows configurable text in the GITS HUD (`Text` in `config.yml`). |
| `Benchmark` | `benchmark` | Writes CPU present-to-present frame times to a CSV file. |
| `RtasCache` | `rtas_cache` | Caches BLAS data via `CopyRaytracingAccelerationStructure` to reduce rebuild cost. |
| `RtasSizeCheck` | `rtas_size_check` | Compares RTAS prebuild sizes between capture and replay on `GetRaytracingAccelerationStructurePrebuildInfo`. |
| `AdapterSpoof` | `adapter_spoof` | Overrides fields from `IDXGIAdapter::GetDesc` for vendor or device testing. |
| `PlatformPortability` | `platform_portability` | Skips or adjusts calls that may be missing on the replay platform. |
| `Statistics` | `statistics` | Aggregates DirectX API call statistics to a YAML report. |

**Benchmark** measures CPU time between presents. **Statistics** counts API usage. They are complementary, not interchangeable.


