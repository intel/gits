---
icon: material/rocket-launch
title: Launcher
---


![GITS Launcher: Banner](../assets/images/launcher/GitsLauncherBanner.png){width="600"} 
/// caption
///

The **GITS Launcher** is a GUI application that handles all required steps in the background to capture, playback and subcapture a GITS stream. It provides a UI for the most frequently used config options, along with ability to edit the full config file including validation.



# Typical workflows

This section gives a brief step-by-step overview on how to use the **GITS Launcher**.

--8<-- "launcher_steps.md"

# GITS Launcher in detail

This section discusses the details of the GITS Launcher UI.

## UI Overview

The **GITS Launcher** consists of several distinct UI sections:  

- A **top bar** containaing the GITS Menu, the mode selector and the main action button(s).
- A mode specific **source panel** that specifies target application, source stream, output folder, ... .
- The **main window area** devided in
  - a **left sidebar** with mode specific entries that switch through various panels and
  - the **main content panel area** that shows the content of the panel selected on the right side.

![Capture configuration options](../assets/images/launcher/Capture_options.png)
/// caption
A screenshot of the default view when starting the **GITS Launcher**
///

## Sidebar Panels

This section offers a description of all panels that can be reached via the left sidebar in the main window area. Not all panels are available in all modes and the panel content depends on the mode and selected API. A panel can have multiple tabs to view certain aspects.

### Configuration

The configuration panel offers a user interface to edit two aspects of gits:

- most used configuration options
- plugins that are run alongside gits.

#### Options

This panel contains the most frequently used or most useful configuration options.

![Configuration options panel in Capture Mode, DirectX](../assets/images/launcher/Capture_options.png)
/// caption
The configuration options panel in capture mode with API set to DirectX 12
///

#### Plugins

The plugins panel allows the enable API-specific plugins as well as editing the associated YAML-configuration file (*without validation*).

![Configuration options plugins in Capture Mode, DirectX](../assets/images/launcher/Capture_plugins.png)
/// caption
The configuration plugins panel in capture mode with API set to DirectX 12
///

### YAML Config

The YAML Config editor allows to edit the full config file with all options. The editoer provides syntax highlighting and validates the file while editing. It contains two tabs:

- the first tab shows the current configuration that will be used when GITS is running.
- the second tab is a read only view of the selected (file path set in the **source panel**) or the default gits configuration.

To better see the changes between the default (or selected) and the current in-memory configuration the **In-Memory Config**-tab provides a difference mode to only show differences.

#### In-Memory Config

![YAML config In-Memory panel](../assets/images/launcher/Capture_config_delta.png)
/// caption
The In-Memory Config editor in the YAML config panel showing *the difference*
///

#### gits_config.yml

The name of the tab shows the filename of the currently selected gits config file, falling to `gits_config.yml` by default.

![YAML config selected/default panel](../assets/images/launcher/Capture_config.png)
/// caption
The read-only viewer of the selected or default `gits_config.yml`.
///

### Resource Dump

The **Resource Dump** panel allows the user to dump resources of the selected stream as described in the [documentation](DirectX/ResourceDumping.md). There are various resources that can be dumped:

- Resources: selectable via `ResourceKeys`, `CommandKeys` using a rescale range in a selectable format.
- RenderTargets: defined by a `Frame range` or `Draw range` in a selectable format.
- DispatchOutputs: defined by a `Frame range` or `Dispatch range` in a selectable format.
- Raytracing: defined by `CommandKeys` and `CommandListModulStep` with `BindingTables`, `Instances` and `Blases`.
- ExecuteIndirect: defined by `CommandKeys` with `ArgumentBuffers`.
- RootSignature: defined by `RootSignatureKeys`.

![Resource dump panel in playback mode](../assets/images/launcher/Playback_resources.png)
/// caption
The resource dump panel with selected `Resources` and `RenderTargets` options.
///

### Metadata

The **Metadata** panel shows the metadata that is baked into a GITS stream - the configuration that was used to create it as well as dignostics data from the system it was created on.

#### Metadata - Configuration

![Metadata configuration panel in playback mode](../assets/images/launcher/Playback_metadata_config.png)
/// caption
Metadata embedded in the stream: the configuration that was used to capture the loaded stream.
///

#### Metadata - Diagnostics

![MMetadata diagnostics panel in playback mode](../assets/images/launcher/Playback_metadata_diagnostic.png)
/// caption
Metadata embedded in the stream: the diagnostics data that was stored when the loaded stream was captured.
///

### GITS & Launcher LOG

There are two log panels:

- **GITS Log** shows the output of GITS itself. This is helpful when there are issues with GITS itself, e.g. capturing or playing back.
- **Launcher Log** is the log of the GITS Launcher and can help track down issues if the GITS operations aren't executed as expected.

### UI Settings

At the bottom of the sidebar there's a section that can be used to change the look of the UI.

![UI settings](../assets/images/launcher/UISettings.png){width="200"}
/// caption
UI settings
///

The UI can dynamically scale to accommodate for various resolutions and currently features a light and a dark mode.

## Launcher Menu


![GITS Launcher Menu](../assets/images/launcher/LauncherMenu.png){width="240"}
/// caption
GITS Launcher Menu
///

The GITS Launcher Menu provides shortcuts to open the currently setup paths in external applications (e.g. Windows: explorer).
