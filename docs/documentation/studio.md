---
icon: material/toolbox
title: Studio
---

**GITS Studio** is a tool to work with live workloads as well as GITS streams and is designed to be extensible by **GITS Modules**, which in turn are can utilize a **GITS Plugins**.

## Stream Mode

In Stream Mode **GITS Studio** can open a GITS stream and show the embedded metadata and can create a textual API Trace. Additionally it provides enhanced playback control - allowing the user to skip forward `x` frames, play and pause the playback as well as selectipvely stepping forward `y` frames.

## Live Workload

To launch a live workload the executable can be selected and additonal launch parameters can be setup. In order to usse the correct Interposer the API of the application has to be selected.

## GITS Modules

During start the **GITS Studio** scans a subfolder called `Modules`, assuming the following structure:

--8<-- "modules_folder.md"

Every `GITS Module` identifies with a unique ID, provides a name as well as a (short) descriptive text which are ussed in the UI. A **GITS Module** needs to be activated.
