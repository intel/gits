---
icon: material/rocket-launch
title: Launcher
---


![GITS Launcher: Banner](../assets/images/launcher/GitsLauncherBanner.png){width="600"} 
/// caption
///

The **GITS Launcher** provides a simple way to **record gits streams** as well as use **gits's playback** and **subcapture functionality** with a **graphical user interface**.

!!! info

    GITS Launcher is currently designed to support DirectX. Not all features work for other APIs.

# Overview

It currently has the following features:  

- Choose the *gits distribution/executables* as well as the *configuration*.  
- A *text editor* for the current *configuration* along with the ability to *validate* it.  
- A *GUI* for *frequently used configuration options*.  
- Getting a *trace, stream stats & diagnostics* as well as *subcapturing* is abstracted to *simple button presses*.

## General UI Overview

Here's a general overview of GITS Launcher:

![Playback Mode: editing the config](../assets/images/launcher/Playback_config.png){width="888"}
/// caption
The main playback mode view, editing the configuration.
///

Here are the main elements (top down):

1. Main Bar:
   1. GITS Launcher Menu
   2. Mode selector
   3. Action Button
2. GITS directories panel  
   Select the GITS install directory or a custom player.
3. Mode-specific panel  
   - Capture: workload + arguments 
   - Playback: stream + custom player arguments 
   - Subcapture: subcapture options
4. Main Panel  
   1. Vertical tabs  
      1. Textual config editing  
      2. Log output of the last GITS run  
      3. Log output of the GITS Launcher  
      4. UI Scale  
      5. UI Theme  
   2. Content panel, based on the selected tab

# Typical workflows

## Capture an application

![Capture Mode: editing the config](../assets/images/launcher/Capture_config.png){width="888"}
/// caption
The main capture mode view, editing the configuration.
///

To capture an application:

1. Select the capture mode.
2. Ensure the gits base path is set to the install directory (to find the capture dlls).
3. Choose the application to capture:
   - Set the proper API.
   - Set commandline arguments if needed.
4. Select the output path where to store the captured stream.
5. Adjust the configuration as desired.
6. Start capturing

## Playback a stream

![Capture Mode: editing the config](../assets/images/launcher/Playback_options.png){width="888"}
/// caption
The main playback mode view, using UI config options.
///

To playback a gits stream:

1. Select the playback mode.
2. Ensure the gits base path is set to the install directory 
   - or you've got a custom gitsPlayer selected.
3. Choose the stream to playback.
4. Setup the config as needed:
   - Either by using the UI config options or
   - Editing the configuration textually. Don't forget to validate.
5. Start the playback.

## Subcapture a stream

!!! info

    Subcapturing using the GITS Launcher currently **only works with  DirectX**.

![Subcapture Mode: editing the config](../assets/images/launcher/Subcapture_config.png){width="888"}
/// caption
The main subcapture mode view, editing the configuration.
///

To subcapture a gits stream:

1. Select the subcapture mode.
2. Ensure the gits base path is set to the install directory 
   - or you've got a custom gitsPlayer selected.
3. Choose the stream to subcapture.
4. Select the subcapture path.
5. Setup the config as needed:
   - Either by using the UI config options or
   - Editing the configuration textually. Don't forget to validate.
6. Start subcapturing. Note that it will playback the stream twice.

# Details

This section introduces the details of the GITS Launcher UI.

## Config editor

![Capture Mode: editing the config](../assets/images/launcher/ConfigEditor.png){width="888"}
/// caption
The Config editor
///

The toolbar buttons on the left side jump directly to top level sections of the config file.  
The buttons on the right side are mostly self-explanatory.  

The rightmost `Check` button checks if the current content of the editor is a valid config file. The button turns green on success or red on failure - until the config editor content is changed. The validation result can also be read in the GITS Launcher Log (`Main Panel > Tabs > Launcher Log`). 

## Options for playback & subcapture

There are specific panels for playback and subcapture mode that contain a ui to easily change frequently used and/or important configuration options.

### Playback options

![Capture Mode: capture options](../assets/images/launcher/Playback_options.png){width="888"}
/// caption
Capture options
///

The Capture options allow the user to: 

- Turn the HUD on/off,
- Enable taking screenshots, including ranges with step size into a specific capture path, and
- Dump a trace to a target folder.

### Subcapture options

![Subapture Mode: subcapture options](../assets/images/launcher/Subcapture_options.png){width="888"}
/// caption
Subcapture options
///

The subcapture options allow the user to:

- Specify the subcapture range and the output folder,
- Disable the subcapture optimization, and
- Serialize CPU and GPU execution.

## Launcher Menu


![GITS Launcher Menu](../assets/images/launcher/LauncherMenu.png){width="240"}
/// caption
GITS Launcher Menu
///

The GITS Launcher Menu provides shortcuts to open the currently setup paths in external applications (e.g. Windows: explorer).

## UI settings

![UI settings](../assets/images/launcher/UISettings.png){width="200"}
/// caption
UI settings
///

The UI can dynamically scale to accommodate for various resolutions and features a light and a dark mode.